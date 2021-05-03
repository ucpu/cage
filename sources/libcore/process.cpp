#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/process.h>
#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "incWin.h"
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <cstdlib>
#include <cerrno>
#include <string>

namespace cage
{
	ProcessCreateConfig::ProcessCreateConfig(const string &cmd, const string &workingDirectory) : cmd(cmd), workingDirectory(workingDirectory)
	{}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS

		class ProcessImpl : public Process
		{
			struct AutoHandle : private cage::Immovable
			{
				HANDLE handle = 0;

				void close()
				{
					if (handle)
					{
						::CloseHandle(handle);
						handle = 0;
					}
				}

				~AutoHandle()
				{
					close();
				}
			};

		public:
			explicit ProcessImpl(const ProcessCreateConfig &config) : cmd(config.cmd), workingDir(pathToAbs(config.workingDirectory))
			{
				static Holder<Mutex> mut = newMutex();
				ScopeLock lock(mut);

				CAGE_LOG(SeverityEnum::Info, "process", stringizer() + "launching process '" + cmd + "'");
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "process", stringizer() + "working directory '" + workingDir + "'");

				SECURITY_ATTRIBUTES saAttr;
				detail::memset(&saAttr, 0, sizeof(SECURITY_ATTRIBUTES));
				saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
				saAttr.bInheritHandle = true;

				if (!CreatePipe(&hPipeOutR.handle, &hPipeOutW.handle, &saAttr, 0))
					CAGE_THROW_ERROR(SystemError, "CreatePipe", GetLastError());

				if (!CreatePipe(&hPipeInR.handle, &hPipeInW.handle, &saAttr, 0))
					CAGE_THROW_ERROR(SystemError, "CreatePipe", GetLastError());

				hFileNull.handle = CreateFile("nul:", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, OPEN_EXISTING, 0, NULL);

				PROCESS_INFORMATION piProcInfo;
				detail::memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));

				STARTUPINFO siStartInfo;
				detail::memset(&siStartInfo, 0, sizeof(STARTUPINFO));
				siStartInfo.cb = sizeof(STARTUPINFO);
				siStartInfo.hStdError = config.discardStdErr ? hFileNull.handle : hPipeOutW.handle;
				siStartInfo.hStdOutput = config.discardStdOut ? hFileNull.handle : hPipeOutW.handle;
				siStartInfo.hStdInput = config.discardStdIn ? hFileNull.handle : hPipeInR.handle;
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
					CAGE_THROW_ERROR(SystemError, "CreateProcess", GetLastError());
				}

				hProcess.handle = piProcInfo.hProcess;
				hThread.handle = piProcInfo.hThread;

				hPipeOutW.close();
				hPipeInR.close();

				CAGE_LOG_CONTINUE(SeverityEnum::Info, "process", stringizer() + "process id: " + (uint32)GetProcessId(hProcess.handle));
			}

			void terminate()
			{
				TerminateProcess(hProcess.handle, 1);
			}

			int wait()
			{
				if (WaitForSingleObject(hProcess.handle, INFINITE) != WAIT_OBJECT_0)
					CAGE_THROW_ERROR(Exception, "WaitForSingleObject");
				DWORD ret = 0;
				if (GetExitCodeProcess(hProcess.handle, &ret) == 0)
					CAGE_THROW_ERROR(SystemError, "GetExitCodeProcess", GetLastError());
				return ret;
			}

			void read(void *data, uint32 size)
			{
				DWORD read = 0;
				if (!ReadFile(hPipeOutR.handle, data, size, &read, nullptr))
				{
					const DWORD err = GetLastError();
					if (err == ERROR_BROKEN_PIPE)
						CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
					CAGE_THROW_ERROR(SystemError, "ReadFile", err);
				}
				if (read != size)
					CAGE_THROW_ERROR(Exception, "insufficient data");
			}

			void write(const void *data, uint32 size)
			{
				DWORD written = 0;
				if (!WriteFile(hPipeInW.handle, data, size, &written, nullptr))
				{
					const DWORD err = GetLastError();
					if (err == ERROR_BROKEN_PIPE)
						CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
					CAGE_THROW_ERROR(SystemError, "WriteFile", err);
				}
				if (written != size)
					CAGE_THROW_ERROR(Exception, "data truncated");
			}

			const string cmd;
			const string workingDir;
			AutoHandle hPipeInR;
			AutoHandle hPipeInW;
			AutoHandle hPipeOutR;
			AutoHandle hPipeOutW;
			AutoHandle hFileNull;
			AutoHandle hProcess;
			AutoHandle hThread;
		};

#else

		class ProcessImpl : public Process
		{
			constexpr static int PIPE_READ = 0;
			constexpr static int PIPE_WRITE = 1;

			struct AutoPipe : private cage::Immovable
			{
				int handles[2] = { -1, -1 };

				void close(int i)
				{
					if (handles[i] >= 0)
					{
						::close(handles[i]);
						handles[i] = -1;
					}
				}

				void close()
				{
					close(0);
					close(1);
				}

				~AutoPipe()
				{
					close();
				}
			};

			static void alterEnvironment()
			{
				const std::string p = getenv("PATH");
				const std::string n = p + ":" + pathWorkingDir().c_str() + ":" + pathExtractDirectory(detail::getExecutableFullPath()).c_str();
				setenv("PATH", n.c_str(), 1);
			}

		public:
			const string cmd;
			const string workingDir;
			AutoPipe aStdinPipe;
			AutoPipe aStdoutPipe;
			AutoPipe aFileNull;
			pid_t pid = -1;

			explicit ProcessImpl(const ProcessCreateConfig &config) : cmd(config.cmd), workingDir(pathToAbs(config.workingDirectory))
			{
				static Holder<Mutex> mut = newMutex();
				ScopeLock lock(mut);

				CAGE_LOG(SeverityEnum::Info, "process", stringizer() + "launching process '" + cmd + "'");
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "process", stringizer() + "working directory '" + workingDir + "'");

				if (pipe(aStdinPipe.handles) < 0)
					CAGE_THROW_ERROR(SystemError, "failed to create pipe", errno);
				if (pipe(aStdoutPipe.handles) < 0)
					CAGE_THROW_ERROR(SystemError, "failed to create pipe", errno);

				aFileNull.handles[0] = open("/dev/null", O_RDWR);
				if (aFileNull.handles[0] < 0)
					CAGE_THROW_ERROR(SystemError, "failed to open null device file", errno);

				pid = fork();

				if (pid == 0)
				{
					// child process

					// redirect stdin
					if (dup2(config.discardStdIn ? aFileNull.handles[0] : aStdinPipe.handles[PIPE_READ], STDIN_FILENO) == -1)
						CAGE_THROW_ERROR(SystemError, "failed to duplicate stdin pipe", errno);

					// redirect stdout
					if (dup2(config.discardStdOut ? aFileNull.handles[0] : aStdoutPipe.handles[PIPE_WRITE], STDOUT_FILENO) == -1)
						CAGE_THROW_ERROR(SystemError, "failed to duplicate stdout pipe", errno);

					// redirect stderr
					if (dup2(config.discardStdErr ? aFileNull.handles[0] : aStdoutPipe.handles[PIPE_WRITE], STDERR_FILENO) == -1)
						CAGE_THROW_ERROR(SystemError, "failed to duplicate stderr pipe", errno);

					// close unused file descriptors
					aStdinPipe.close(PIPE_READ);
					aStdinPipe.close(PIPE_WRITE);
					aStdoutPipe.close(PIPE_READ);
					aStdoutPipe.close(PIPE_WRITE);

					// alter environment
					alterEnvironment();

					// run child process image
					const string params = string() + "(cd '" + workingDir + "'; " + cmd + " )";
					const int res = execlp("/bin/sh", "sh", "-c", params.c_str(), nullptr);

					// if we get here, an error occurred, but we are in the child process, so just exit
					exit(res);
				}
				else if (pid > 0)
				{
					// parent process

					// close unused file descriptors
					aStdinPipe.close(PIPE_READ);
					aStdoutPipe.close(PIPE_WRITE);
				}
				else
				{
					// failed to create child
					CAGE_THROW_ERROR(SystemError, "fork failed", errno);
				}

				CAGE_LOG_CONTINUE(SeverityEnum::Info, "process", stringizer() + "process id: " + pid);
			}

			~ProcessImpl()
			{
				wait();
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
						const int res = waitpid(pid, &status, 0);
						const int err = errno;
						if (res < 0 && err == EINTR)
							continue;
						if (res != pid)
							CAGE_THROW_ERROR(SystemError, "waitpid", err);
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

			void read(void *data, uint32 size)
			{
				if (size == 0)
					return;
				const auto r = ::read(aStdoutPipe.handles[PIPE_READ], data, size);
				if (r < 0)
					CAGE_THROW_ERROR(SystemError, "read", errno);
				if (r == 0)
					CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
				if (r != size)
					CAGE_THROW_ERROR(Exception, "insufficient data");
			}

			void write(const void *data, uint32 size)
			{
				if (size == 0)
					return;
				const auto r = ::write(aStdinPipe.handles[PIPE_WRITE], data, size);
				if (r < 0)
					CAGE_THROW_ERROR(SystemError, "write", errno);
				if (r == 0)
					CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
				if (r != size)
					CAGE_THROW_ERROR(Exception, "data truncated");
			}
		};

#endif
	}

	void Process::read(PointerRange<char> buffer)
	{
		ProcessImpl *impl = (ProcessImpl*)this;
		impl->read(buffer.data(), numeric_cast<uint32>(buffer.size()));
	}

	Holder<PointerRange<char>> Process::read(uintPtr size)
	{
		MemoryBuffer buf(size);
		read(buf);
		return std::move(buf);
	}

	string Process::readLine()
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		string out;
		char buffer[string::MaxLength + 1];
		uintPtr size = 0;
		while (size < string::MaxLength)
		{
			impl->read(buffer + size, 1);
			size++;
			PointerRange<const char> pr = { buffer, buffer + size };
			if (detail::readLine(out, pr, true))
				return out;
		}
		CAGE_THROW_ERROR(Exception, "line too long");
	}

	void Process::write(PointerRange<const char> buffer)
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		impl->write(buffer.data(), numeric_cast<uint32>(buffer.size()));
	}

	void Process::writeLine(const string &line)
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		write({ line.c_str(), line.c_str() + line.length() });
		const char eol[2] = "\n";
		write({ eol, eol + 1, });
	}

	void Process::terminate()
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		impl->terminate();
	}

	int Process::wait()
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		return impl->wait();
	}

	string Process::getCmdString() const
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		return impl->cmd;
	}

	string Process::getWorkingDir() const
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		return impl->workingDir;
	}

	Holder<Process> newProcess(const ProcessCreateConfig &config)
	{
		return systemArena().createImpl<Process, ProcessImpl>(config);
	}
}
