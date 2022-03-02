#include <cage-core/concurrent.h>
#include <cage-core/process.h>
#include <cage-core/lineReader.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "incWin.h"
#else
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#include <cstdlib>
#include <cerrno>
#include <string>

namespace cage
{
	ProcessCreateConfig::ProcessCreateConfig(const String &command, const String &workingDirectory) : command(command), workingDirectory(workingDirectory)
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
			explicit ProcessImpl(const ProcessCreateConfig &config)
			{
				static Holder<Mutex> mut = newMutex();
				ScopeLock lock(mut);

				CAGE_LOG(SeverityEnum::Info, "process", Stringizer() + "launching process '" + config.command + "'");

				const String workingDir = pathToAbs(config.workingDirectory);
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "process", Stringizer() + "working directory '" + workingDir + "'");

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

				char cmd2[String::MaxLength];
				detail::memset(cmd2, 0, String::MaxLength);
				detail::memcpy(cmd2, config.command.c_str(), config.command.length());

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

				CAGE_LOG_CONTINUE(SeverityEnum::Info, "process", Stringizer() + "process id: " + (uint32)GetProcessId(hProcess.handle));
			}

			~ProcessImpl()
			{
				wait();
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

			void error(StringLiteral msg)
			{
				const DWORD err = GetLastError();
				if (err == ERROR_BROKEN_PIPE)
					CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
				CAGE_THROW_ERROR(SystemError, msg, err);
			}

			void read(PointerRange<char> buffer) override
			{
				DWORD read = 0;
				if (!ReadFile(hPipeOutR.handle, buffer.data(), numeric_cast<DWORD>(buffer.size()), &read, nullptr))
					error("ReadFile");
				if (read != buffer.size())
					CAGE_THROW_ERROR(Exception, "insufficient data");
			}

			void write(PointerRange<const char> buffer) override
			{
				DWORD written = 0;
				if (!WriteFile(hPipeInW.handle, buffer.data(), numeric_cast<DWORD>(buffer.size()), &written, nullptr))
				{
					const DWORD err = GetLastError();
					if (err == ERROR_BROKEN_PIPE)
						CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
					CAGE_THROW_ERROR(SystemError, "WriteFile", err);
				}
				if (written != buffer.size())
					CAGE_THROW_ERROR(Exception, "data truncated");
			}

			uintPtr size() override
			{
				DWORD sz = 0;
				if (!PeekNamedPipe(hPipeOutR.handle, nullptr, 0, nullptr, &sz, nullptr))
					error("PeekNamedPipe");
				return sz;
			}

			String readLine() override
			{
				char buffer[String::MaxLength + 1]; // plus 1 to allow detecting that the line is too long
				uintPtr size = 0;
				while (true)
				{
					read({ buffer + size, buffer + size + 1 });
					size++;
					String line;
					if (detail::readLine(line, { buffer, buffer + size }, true))
						return line;
				}
			}

			/* this is disabled for compatibility with linux
			bool readLine(string &line) override
			{
				char buffer[string::MaxLength + 1]; // plus 1 to allow detecting that the line is too long
				DWORD sz = 0;
				if (!PeekNamedPipe(hPipeOutR.handle, buffer, sizeof(buffer), &sz, nullptr, nullptr))
					error("PeekNamedPipe");
				const uintPtr l = detail::readLine(line, { buffer, buffer + sz }, true);
				if (l)
					read({ buffer, buffer + l });
				return l;
			}
			*/

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

		public:
			AutoPipe aStdinPipe;
			AutoPipe aStdoutPipe;
			AutoPipe aFileNull;
			pid_t pid = -1;

			explicit ProcessImpl(const ProcessCreateConfig &config)
			{
				static Holder<Mutex> mut = newMutex();
				ScopeLock lock(mut);

				CAGE_LOG(SeverityEnum::Info, "process", Stringizer() + "launching process '" + config.command + "'");

				const String workingDir = pathToAbs(config.workingDirectory);
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "process", Stringizer() + "working directory '" + workingDir + "'");

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
					aStdinPipe.close();
					aStdoutPipe.close();

					{ // alter environment PATH
						const std::string p = getenv("PATH");
						const std::string n = p + ":" + pathWorkingDir().c_str() + ":" + pathExtractDirectory(detail::executableFullPath()).c_str();
						if (setenv("PATH", n.c_str(), 1) != 0)
							CAGE_THROW_ERROR(SystemError, "failed to change environment PATH variable", errno);
					}

					// new working directory
					if (chdir(workingDir.c_str()) != 0)
						CAGE_THROW_ERROR(SystemError, "failed to change working directory", errno);

					// run child process image
					// using bin/sh to automatically split individual arguments
					const int res = execlp("/bin/sh", "sh", "-c", config.command.c_str(), nullptr);

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

				CAGE_LOG_CONTINUE(SeverityEnum::Info, "process", Stringizer() + "process id: " + pid);
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

			void read(PointerRange<char> buffer) override
			{
				if (buffer.size() == 0)
					return;
				const auto r = ::read(aStdoutPipe.handles[PIPE_READ], buffer.data(), buffer.size());
				if (r < 0)
					CAGE_THROW_ERROR(SystemError, "read", errno);
				if (r == 0)
					CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
				if (r != buffer.size())
					CAGE_THROW_ERROR(Exception, "insufficient data");
			}

			void write(PointerRange<const char> buffer) override
			{
				if (buffer.size() == 0)
					return;
				const auto r = ::write(aStdinPipe.handles[PIPE_WRITE], buffer.data(), buffer.size());
				if (r < 0)
					CAGE_THROW_ERROR(SystemError, "write", errno);
				if (r == 0)
					CAGE_THROW_ERROR(ProcessPipeEof, "pipe eof");
				if (r != buffer.size())
					CAGE_THROW_ERROR(Exception, "data truncated");
			}

			uintPtr size() override
			{
				int s = -1;
				const auto r = ::ioctl(aStdoutPipe.handles[PIPE_READ], FIONREAD, &s);
				if (r < 0)
					CAGE_THROW_ERROR(SystemError, "ioctl", errno);
				CAGE_ASSERT(s >= 0);
				return s;
			}

			String readLine() override
			{
				char buffer[String::MaxLength + 1]; // plus 1 to allow detecting that the line is too long
				uintPtr size = 0;
				while (true)
				{
					read({ buffer + size, buffer + size + 1 });
					size++;
					String line;
					if (detail::readLine(line, { buffer, buffer + size }, true))
						return line;
				}
			}

			/* this would require file peek which is not possible with pipes
			bool readLine(string &line) override
			{}
			*/
		};

#endif
	}

	void Process::terminate()
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		impl->terminate();
	}

	sint32 Process::wait()
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		return impl->wait();
	}

	Holder<Process> newProcess(const ProcessCreateConfig &config)
	{
		return systemMemory().createImpl<Process, ProcessImpl>(config);
	}
}
