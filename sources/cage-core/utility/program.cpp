#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/program.h>
#include "../system.h"

namespace cage
{
	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS

		class programImpl : public programClass
		{
		public:
			programImpl(const string &cmd, const string &workingDir) : cmd(cmd), workingDir(workingDir), hChildStd_IN_Rd(nullptr), hChildStd_IN_Wr(nullptr), hChildStd_OUT_Rd(nullptr), hChildStd_OUT_Wr(nullptr), /*hChildStd_ERR_Rd (nullptr), hChildStd_ERR_Wr (nullptr),*/ hProcess(nullptr), hThread(nullptr)
			{
				static holder<mutexClass> mut = newMutex();
				scopeLock<mutexClass> lock(mut);

				CAGE_LOG(severityEnum::Info, "program", string() + "launching program '" + cmd + "' in directory '" + workingDir + "'");

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

				CAGE_LOG_CONTINUE(severityEnum::Info, "program", string() + "process id: " + templates::underlying_type<DWORD>::type(GetProcessId(hProcess)));
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

		class programImpl : public programClass
		{
		public:
			const string cmd;
			const string workingDir;
			int aStdinPipe[2];
			int aStdoutPipe[2];
			pid_t pid;
			
			class envAlt
			{
				string p;

			public:
				envAlt()
				{
					p = getenv("PATH");
					setenv("PATH", (string() + ".:" + p).c_str(), 1);
				}
				
				~envAlt()
				{
					setenv("PATH", p.c_str(), 1);
				}
			};

			programImpl(const string &cmd, const string &workingDir) : cmd(cmd), workingDir(workingDir), aStdinPipe{0, 0}, aStdoutPipe{0, 0}, pid(0)
			{
				static holder<mutexClass> mut = newMutex();
				scopeLock<mutexClass> lock(mut);

				CAGE_LOG(severityEnum::Info, "program", string() + "launching program '" + cmd + "' in directory '" + workingDir + "'");
				
				envAlt envalt;
				
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

				CAGE_LOG_CONTINUE(severityEnum::Info, "program", string() + "process id: " + pid);
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
				errno = 0;
				if (waitpid(pid, &status, 0) != pid)
					CAGE_THROW_ERROR(codeException, "waitpid", errno);
				pid = 0;
				return status;
			}
		};

#endif
	}

#ifdef CAGE_SYSTEM_WINDOWS

	void programClass::read(void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		DWORD read = 0;
		if (!ReadFile(impl->hChildStd_OUT_Rd, data, size, &read, nullptr))
			CAGE_THROW_ERROR(codeException, "ReadFile", GetLastError());
		if (read != size)
			CAGE_THROW_ERROR(exception, "insufficient data");
	}

	void programClass::write(const void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		DWORD written = 0;
		if (!WriteFile(impl->hChildStd_IN_Wr, data, size, &written, nullptr))
			CAGE_THROW_ERROR(codeException, "WriteFile", GetLastError());
		if (written != size)
			CAGE_THROW_ERROR(exception, "data truncated");
	}

#else

	void programClass::read(void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		auto r = ::read(impl->aStdoutPipe[PIPE_READ], data, size);
		if (r < 0)
			CAGE_THROW_ERROR(codeException, "read", errno);
		if (r != size)
			CAGE_THROW_ERROR(exception, "insufficient data");
	}

	void programClass::write(const void *data, uint32 size)
	{
		programImpl *impl = (programImpl*)this;
		auto r = ::write(impl->aStdinPipe[PIPE_WRITE], data, size);
		if (r < 0)
			CAGE_THROW_ERROR(codeException, "write", errno);
		if (r != size)
			CAGE_THROW_ERROR(exception, "data truncated");
	}

#endif

	string programClass::readLine()
	{
		string res;
		while (true)
		{
			char tmp;
			try
			{
				read(&tmp, 1);
			}
			catch (const exception &)
			{
				return res;
			}
			if (tmp == '\n')
			{
				if (!res.empty() && res[res.length() - 1] == 13)
					res = res.subString(0, res.length() - 1);
				if (res.empty())
					continue;
				return res;
			}
			if (res.length() + 1 == string::MaxLength)
				CAGE_THROW_ERROR(exception, "line too long");
			res += string(&tmp, 1);
		}
		return res;
	}

	void programClass::writeLine(const string &data)
	{
		string d = data + "\n";
		write(d.c_str(), d.length());
	}

	void programClass::terminate()
	{
		programImpl *impl = (programImpl*)this;
		impl->terminate();
	}

	int programClass::wait()
	{
		programImpl *impl = (programImpl*)this;
		return impl->wait();
	}

	string programClass::getCmdString() const
	{
		programImpl *impl = (programImpl*)this;
		return impl->cmd;
	}

	string programClass::getWorkingDir() const
	{
		programImpl *impl = (programImpl*)this;
		return impl->workingDir;
	}

	holder<programClass> newProgram(const string &cmd)
	{
		return detail::systemArena().createImpl<programClass, programImpl>(cmd, pathWorkingDir());
	}

	holder<programClass> newProgram(const string &cmd, const string &workingDirectory)
	{
		return detail::systemArena().createImpl<programClass, programImpl>(cmd, pathToAbs(workingDirectory));
	}
}
