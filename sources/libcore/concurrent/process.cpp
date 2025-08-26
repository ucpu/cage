#include <cerrno>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef CAGE_SYSTEM_WINDOWS
	#include "../windowsMinimumInclude.h"
	#include <csignal>
	#include <psapi.h>
#else
	#include <fcntl.h>
	#include <csignal>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <unistd.h>
#endif

#include <cage-core/concurrent.h>
#include <cage-core/debug.h>
#include <cage-core/lineReader.h>
#include <cage-core/process.h>

#ifdef CAGE_SYSTEM_WINDOWS
extern "C"
{
	CAGE_API_EXPORT DWORD WINAPI CageRemoteThreadEntryPointToRaiseSignalTerm(_In_ LPVOID)
	{
		try
		{
			using namespace cage;
			CAGE_LOG(SeverityEnum::Warning, "process", "raising SIGTERM");
		}
		catch (...)
		{
			// nothing
		}
		raise(SIGTERM);
		return 0;
	}
}
#endif

namespace cage
{
	ProcessCreateConfig::ProcessCreateConfig(const String &command, const String &workingDirectory) : command(command), workingDirectory(workingDirectory) {}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS

		static constexpr const char *CageCoreName = CAGE_DEBUG_BOOL ? "cage-cored.dll" : "cage-core.dll";

		uintPtr baseAddressInProcess(HANDLE hProcess)
		{
			std::vector<HMODULE> modules;
			DWORD size = 1024 * sizeof(HMODULE);
			modules.resize(size / sizeof(HMODULE));
			if (!EnumProcessModulesEx(hProcess, modules.data(), size, &size, sizeof(uintPtr) == 4 ? LIST_MODULES_32BIT : LIST_MODULES_64BIT))
				CAGE_THROW_ERROR(SystemError, "EnumProcessModules", GetLastError());
			modules.resize(size / sizeof(HMODULE));
			for (HMODULE h : modules)
			{
				char name[MAX_PATH] = {};
				if (GetModuleBaseName(hProcess, h, name, sizeof(name)))
				{
					if (String(name) == CageCoreName)
					{
						MODULEINFO info = {};
						GetModuleInformation(hProcess, h, &info, sizeof(info));
						return (uintPtr)info.lpBaseOfDll;
					}
				}
			}
			CAGE_THROW_ERROR(Exception, "cage-core.dll not found in the target process");
		}

		void sendSigTermTo(HANDLE hProcess)
		{
			AutoHandle hRemote;
			hRemote.handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetProcessId(hProcess));
			if (!hRemote.handle)
				CAGE_THROW_ERROR(SystemError, "OpenProcess", GetLastError());
			const uintPtr remoteBase = baseAddressInProcess(hProcess);

			AutoHandle hMyself;
			hMyself.handle = GetCurrentProcess();
			const uintPtr localBase = baseAddressInProcess(hMyself.handle);

			const uintPtr localAddress = (uintPtr)GetProcAddress(GetModuleHandle(CageCoreName), "CageRemoteThreadEntryPointToRaiseSignalTerm");
			if (!localAddress)
				CAGE_THROW_ERROR(SystemError, "GetProcAddress", GetLastError());

			const uintPtr startAddress = localAddress + remoteBase - localBase;
			AutoHandle hThread;
			hThread.handle = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)startAddress, nullptr, 0, nullptr);
			if (!hThread.handle)
				CAGE_THROW_ERROR(SystemError, "CreateRemoteThread", GetLastError());
			WaitForSingleObject(hThread.handle, INFINITE);
		}

		class ProcessImpl : public Process
		{
		public:
			explicit ProcessImpl(const ProcessCreateConfig &config)
			{
				static Holder<Mutex> mut = newMutex();
				ScopeLock lock(mut);

				CAGE_LOG(SeverityEnum::Info, "process", Stringizer() + "launching process: " + config.command);

				const String workingDir = pathToAbs(config.workingDirectory);
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "process", Stringizer() + "working directory: " + workingDir);
				pathCreateDirectories(workingDir);

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

				DWORD flags = 0;
				if (config.priority < 0)
					flags |= BELOW_NORMAL_PRIORITY_CLASS;
				else if (config.priority > 0)
					flags |= ABOVE_NORMAL_PRIORITY_CLASS;
				if (config.detached)
					flags |= DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;

				if (!CreateProcess(nullptr, cmd2, nullptr, nullptr, TRUE, flags, nullptr, workingDir.c_str(), &siStartInfo, &piProcInfo))
					CAGE_THROW_ERROR(SystemError, "CreateProcess", GetLastError());

				hProcess.handle = piProcInfo.hProcess;
				hThread.handle = piProcInfo.hThread;

				CAGE_LOG_CONTINUE(SeverityEnum::Info, "process", Stringizer() + "process id: " + (uint32)GetProcessId(hProcess.handle));

				hPipeOutW.close();
				hPipeInR.close();

				if (config.detached)
				{
					hPipeOutR.close();
					hPipeInW.close();
					hProcess.close();
					hThread.close();
				}
			}

			~ProcessImpl()
			{
				try
				{
					wait();
				}
				catch (...)
				{
					detail::logCurrentCaughtException();
					CAGE_LOG(SeverityEnum::Warning, "process", Stringizer() + "exception thrown in process was not propagated to the caller (missing call to wait)");
					// we continue
				}
			}

			void requestTerminate()
			{
				try
				{
					detail::OverrideBreakpoint ob;
					sendSigTermTo(hProcess.handle);
				}
				catch (...)
				{
					// do nothing
				}
			}

			void terminate() { TerminateProcess(hProcess.handle, 1); }

			int wait()
			{
				if (hProcess.handle == 0)
					return 0;
				if (WaitForSingleObject(hProcess.handle, INFINITE) != WAIT_OBJECT_0)
					CAGE_THROW_ERROR(Exception, "WaitForSingleObject");
				DWORD ret = 0;
				if (GetExitCodeProcess(hProcess.handle, &ret) == 0)
					CAGE_THROW_ERROR(SystemError, "GetExitCodeProcess", GetLastError());
				return ret;
			}

			void error(StringPointer msg)
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

			uint64 size() override
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
			static constexpr int PIPE_READ = 0;
			static constexpr int PIPE_WRITE = 1;

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
					close(PIPE_READ);
					close(PIPE_WRITE);
				}

				~AutoPipe() { close(); }
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

				CAGE_LOG(SeverityEnum::Info, "process", Stringizer() + "launching process: " + config.command);

				const String workingDir = pathToAbs(config.workingDirectory);
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "process", Stringizer() + "working directory: " + workingDir);

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

					// lower priority
					if (config.priority < 0)
					{
						nice(3);
						// ignore errors here
					}
					// higher priority not available on linux

					if (config.detached)
					{
						if (setsid() < 0)
							CAGE_THROW_ERROR(SystemError, "failed to detach process (setsid)", errno);
					}

					// run child process image
					// using bin/sh to automatically split individual arguments
					const int res = execlp("/bin/sh", "sh", "-c", config.command.c_str(), nullptr);

					// if we get here, an error occurred, but we are in the child process, so just exit
					exit(res);
				}
				else if (pid < 0)
				{
					// failed to create child
					CAGE_THROW_ERROR(SystemError, "fork failed", errno);
				}

				// parent process
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "process", Stringizer() + "process id: " + pid);

				// close unused file descriptors
				aStdinPipe.close(PIPE_READ);
				aStdoutPipe.close(PIPE_WRITE);

				if (config.detached)
				{
					aStdinPipe.close(PIPE_WRITE);
					aStdoutPipe.close(PIPE_READ);
					pid = -1;
				}
			}

			~ProcessImpl()
			{
				try
				{
					wait();
				}
				catch (...)
				{
					detail::logCurrentCaughtException();
					CAGE_LOG(SeverityEnum::Warning, "process", Stringizer() + "exception thrown in process was not propagated to the caller (missing call to wait)");
					// we continue
				}
			}

			void requestTerminate() { kill(pid, SIGTERM); }

			void terminate() { kill(pid, SIGKILL); }

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

	void Process::requestTerminate()
	{
		ProcessImpl *impl = (ProcessImpl *)this;
		impl->requestTerminate();
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
		auto r = systemMemory().createImpl<Process, ProcessImpl>(config);
		if (config.detached)
			return {};
		return r;
	}
}
