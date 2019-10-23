#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/process.h>
#include <cage-core/lineReader.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "incWin.h"
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <cstdlib>
#include <cerrno>
#include <string>

namespace cage
{
	processCreateConfig::processCreateConfig(const string &cmd, const string &workingDirectory) : cmd(cmd), workingDirectory(workingDirectory)
	{}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS

		class programImpl : public processHandle
		{
		public:
			programImpl(const processCreateConfig &config) : cmd(config.cmd), workingDir(pathToAbs(config.workingDirectory)), hChildStd_IN_Rd(nullptr), hChildStd_IN_Wr(nullptr), hChildStd_OUT_Rd(nullptr), hChildStd_OUT_Wr(nullptr), /*hChildStd_ERR_Rd (nullptr), hChildStd_ERR_Wr (nullptr),*/ hProcess(nullptr), hThread(nullptr)
			{
				static holder<syncMutex> mut = newSyncMutex();
				scopeLock<syncMutex> lock(mut);

				CAGE_LOG(severityEnum::Info, "process", string() + "launching process '" + cmd + "'");
				CAGE_LOG_CONTINUE(severityEnum::Note, "process", string() + "working directory '" + workingDir + "'");

				SECURITY_ATTRIBUTES saAttr;
				detail::memset(&saAttr, 0, sizeof(SECURITY_ATTRIBUTES));
				saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
				saAttr.bInheritHandle = true;

				if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0))
					CAGE_THROW_ERROR(codeException, "CreatePipe", GetLastError());

				if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0))
					CAGE_THROW_ERROR(codeException, "CreatePipe", GetLastError());

				PROCESS_INFORMATION piProcInfo;
				detail::memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));

				STARTUPINFO siStartInfo;
				detail::memset(&siStartInfo, 0, sizeof(STARTUPINFO));
				siStartInfo.cb = sizeof(STARTUPINFO);
				siStartInfo.hStdError = hChildStd_OUT_Wr;
				siStartInfo.hStdOutput = hChildStd_OUT_Wr;
				siStartInfo.hStdInput = hChildStd_IN_Rd;
				siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

				char cmd2[string::MaxLength];
				detail::memset(cmd2, 0, string::MaxLength);
				detail::memcpy(cmd2, cmd.c_str(), cmd.length());
				if (!CreateProcess(
					nullptr,
					cmd2,
					nullptr,
					nullptr,
					TRUE,
					0,
					nullptr,
					workingDir.c_str(),
					&siStartInfo,
					&piProcInfo
				))
				{
					CAGE_THROW_ERROR(codeException, "CreateProcess", GetLastError());
				}

				hProcess = piProcInfo.hProcess;
				hThread = piProcInfo.hThread;

				closeHandle(hChildStd_OUT_Wr);
				closeHandle(hChildStd_IN_Rd);

				CAGE_LOG_CONTINUE(severityEnum::Info, "process", string() + "process id: " + templates::underlying_type<DWORD>::type(GetProcessId(hProcess)));
			}

			static void closeHandle(HANDLE &h)
			{
				if (h)
				{
					CloseHandle(h);
					h = nullptr;
				}
			}

			void closeAllHandles()
			{
				closeHandle(hChildStd_IN_Rd);
				closeHandle(hChildStd_IN_Wr);
				closeHandle(hChildStd_OUT_Rd);
				closeHandle(hChildStd_OUT_Wr);
				closeHandle(hProcess);
				closeHandle(hThread);
			}

			~programImpl()
			{
				closeAllHandles();
			}

			void terminate()
			{
				TerminateProcess(hProcess, 1);
				closeAllHandles();
			}

			int wait()
			{
				if (WaitForSingleObject(hProcess, INFINITE) != WAIT_OBJECT_0)
					CAGE_THROW_ERROR(exception, "WaitForSingleObject");
				DWORD ret = 0;
				if (GetExitCodeProcess(hProcess, &ret) == 0)
					CAGE_THROW_ERROR(codeException, "GetExitCodeProcess", GetLastError());
				closeAllHandles();
				return ret;
			}

			const string cmd;
			const string workingDir;
			HANDLE hChildStd_IN_Rd;
			HANDLE hChildStd_IN_Wr;
			HANDLE hChildStd_OUT_Rd;
			HANDLE hChildStd_OUT_Wr;
			HANDLE hProcess;
			HANDLE hThread;
		};

#else

		static const int PIPE_READ = 0;
		static const int PIPE_WRITE = 1;

		class programImpl : public processHandle
		{
		public:
			const string cmd;
			const string workingDir;
			int aStdinPipe[2];
			int aStdoutPipe[2];
			pid_t pid;

			class envAlt
			{
				std::string p; // std string has unlimited length

			public:
				envAlt()
				{
					p = getenv("PATH");
					std::string n = p + ":" + pathWorkingDir().c_str() + ":" + pathExtractPath(detail::getExecutableFullPath()).c_str();
					setenv("PATH", n.c_str(), 1);
				}

				~envAlt()
				{
					setenv("PATH", p.c_str(), 1);
				}
			};

			programImpl(const processCreateConfig &config) : cmd(config.cmd), workingDir(pathToAbs(config.workingDirectory)), aStdinPipe{0, 0}, aStdoutPipe{0, 0}, pid(0)
			{
				static holder<syncMutex> mut = newSyncMutex();
				scopeLock<syncMutex> lock(mut);

				CAGE_LOG(severityEnum::Info, "process", string() + "launching process '" + cmd + "'");
				CAGE_LOG_CONTINUE(severityEnum::Note, "process", string() + "working directory '" + workingDir + "'");

				if (pipe(aStdinPipe) < 0)
					CAGE_THROW_ERROR(exception, "failed to open pipe");
				if (pipe(aStdoutPipe) < 0)
				{
					close(aStdinPipe[PIPE_READ]);
					close(aStdinPipe[PIPE_WRITE]);
					CAGE_THROW_ERROR(exception, "failed to open pipe");
				}

				pid = fork();

				if (pid == 0)
				{
					// child process

					// redirect stdin
					if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1)
						CAGE_THROW_ERROR(exception, "failed to duplicate stdin pipe");

					// redirect stdout
					if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1)
						CAGE_THROW_ERROR(exception, "failed to duplicate stdout pipe");

					// redirect stderr
					if (dup2(aStdoutPipe[PIPE_WRITE], STDERR_FILENO) == -1)
						CAGE_THROW_ERROR(exception, "failed to duplicate stderr pipe");

					// close unused file descriptors
					close(aStdinPipe[PIPE_READ]);
					close(aStdinPipe[PIPE_WRITE]);
					close(aStdoutPipe[PIPE_READ]);
					close(aStdoutPipe[PIPE_WRITE]);

					// alter environment
					envAlt envalt;

					// run child process image
					string params = string() + "(cd '" + workingDir + "'; " + cmd + " )";
					int res = execlp("/bin/sh", "sh", "-c", params.c_str(), nullptr);

					// if we get here at all, an error occurred, but we are in the child process, so just exit
					exit(res);
				}
				else if (pid > 0)
				{
					// parent process

					// close unused file descriptors
					close(aStdinPipe[PIPE_READ]);
					close(aStdoutPipe[PIPE_WRITE]);
				}
				else
				{
					// failed to create child
					close(aStdinPipe[PIPE_READ]);
					close(aStdinPipe[PIPE_WRITE]);
					close(aStdoutPipe[PIPE_READ]);
					close(aStdoutPipe[PIPE_WRITE]);
					CAGE_THROW_ERROR(exception, "fork failed");
				}

				CAGE_LOG_CONTINUE(severityEnum::Info, "process", string() + "process id: " + pid);
			}

			~programImpl()
			{
				wait();
				close(aStdinPipe[PIPE_WRITE]);
				close(aStdoutPipe[PIPE_READ]);
			}

			void terminate()
			{
				kill(pid, SIGKILL);
			}

			int wait()
			{
				if (pid <= 0)
					return 0;
				int status = 0;
				try
				{
					while (true)
					{
						errno = 0;
						int res = waitpid(pid, &status, 0);
						int err = errno;
						if (res < 0 && err == EINTR)
							continue;
						if (res != pid)
							CAGE_THROW_ERROR(codeException, "waitpid", err);
						break;
					}
				}
				catch (...)
				{
					pid = 0;
					throw;
				}
				pid = 0;
				return status;
			}
		};

#endif
	}

#ifdef CAGE_SYSTEM_WINDOWS

	void processHandle::read(void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		DWORD read = 0;
		if (!ReadFile(impl->hChildStd_OUT_Rd, data, size, &read, nullptr))
			CAGE_THROW_ERROR(codeException, "ReadFile", GetLastError());
		if (read != size)
			CAGE_THROW_ERROR(exception, "insufficient data");
	}

	void processHandle::write(const void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		DWORD written = 0;
		if (!WriteFile(impl->hChildStd_IN_Wr, data, size, &written, nullptr))
			CAGE_THROW_ERROR(codeException, "WriteFile", GetLastError());
		if (written != size)
			CAGE_THROW_ERROR(exception, "data truncated");
	}

#else

	void processHandle::read(void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		auto r = ::read(impl->aStdoutPipe[PIPE_READ], data, size);
		if (r < 0)
			CAGE_THROW_ERROR(codeException, "read", errno);
		if (r != size)
			CAGE_THROW_ERROR(exception, "insufficient data");
	}

	void processHandle::write(const void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		auto r = ::write(impl->aStdinPipe[PIPE_WRITE], data, size);
		if (r < 0)
			CAGE_THROW_ERROR(codeException, "write", errno);
		if (r != size)
			CAGE_THROW_ERROR(exception, "data truncated");
	}

#endif

	string processHandle::readLine()
	{
		string out;
		char buffer[string::MaxLength + 1];
		uintPtr size = 0;
		while (size < string::MaxLength)
		{
			read(buffer + size, 1);
			size++;
			const char *b = buffer;
			if (detail::readLine(out, b, size, true))
				return out;
		}
		CAGE_THROW_ERROR(exception, "line too long");
	}

	void processHandle::writeLine(const string &data)
	{
		string d = data + "\n";
		write(d.c_str(), d.length());
	}

	void processHandle::terminate()
	{
		programImpl *impl = (programImpl*)this;
		impl->terminate();
	}

	int processHandle::wait()
	{
		programImpl *impl = (programImpl*)this;
		return impl->wait();
	}

	string processHandle::getCmdString() const
	{
		programImpl *impl = (programImpl*)this;
		return impl->cmd;
	}

	string processHandle::getWorkingDir() const
	{
		programImpl *impl = (programImpl*)this;
		return impl->workingDir;
	}

	holder<processHandle> newProcess(const processCreateConfig &config)
	{
		return detail::systemArena().createImpl<processHandle, programImpl>(config);
	}
}
