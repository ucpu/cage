#ifdef CAGE_SYSTEM_LINUX

	#include <csignal>
	#include <cstddef>
	#include <cstdio>
	#include <cstdlib>
	#include <cstring>
	#include <cerrno>
	#include <execinfo.h>
	#include <sys/mman.h>
	#include <ucontext.h>
	#include <unistd.h>

	#include <cage-core/core.h>

namespace cage
{
	int crashHandlerLogFileFd = STDERR_FILENO;

	namespace
	{
		void *altStackMem = nullptr;
		constexpr int OldHandlersCount = 16;
		struct sigaction prevHandlers[OldHandlersCount] = {};

		struct sigaction *prevHandler(int sig)
		{
			if (sig >= 0 && sig < OldHandlersCount)
				return &prevHandlers[sig];
			return nullptr;
		}

		const char *sigToStr(int sig)
		{
			switch (sig)
			{
				case SIGSEGV:
					return "SIGSEGV";
				case SIGABRT:
					return "SIGABRT";
				case SIGFPE:
					return "SIGFPE";
				case SIGILL:
					return "SIGILL";
				case SIGBUS:
					return "SIGBUS";
				case SIGTRAP:
					return "SIGTRAP";
				case SIGTERM:
					return "SIGTERM";
				default:
					return "UNKNOWN";
			}
		}

		void safeWrite(const String &s)
		{
			write(crashHandlerLogFileFd, s.c_str(), s.length());
		}

		void printStackTrace()
		{
			// backtrace (best-effort; not strictly async-signal-safe)
			static constexpr int kMaxFrames = 256;
			void *frames[kMaxFrames];
			int n = backtrace(frames, kMaxFrames);
			safeWrite(Stringizer() + "stack trace:\n");
			backtrace_symbols_fd(frames, n, crashHandlerLogFileFd); // writes directly to fd
		}

		void crashHandler(int sig, siginfo_t *si, void *uctx)
		{
			safeWrite(Stringizer() + "signal caught: " + sigToStr(sig) + " (" + sig + ")\n");

			if (si)
				safeWrite(Stringizer() + "fault addr: " + (uintptr_t)si->si_addr + ", code: " + si->si_code + "\n");

			printStackTrace();

			if (struct sigaction *prev = prevHandler(sig))
			{
				if (prev->sa_flags & SA_SIGINFO)
				{
					// previous handler expects 3 arguments
					prev->sa_sigaction(sig, si, uctx);
				}
				else if (prev->sa_handler == SIG_DFL || prev->sa_handler == SIG_IGN)
				{
					// default or ignore: you can reset signal to default and raise
					signal(sig, prev->sa_handler);
					raise(sig);
				}
				else
				{
					// simple handler with 1 argument
					prev->sa_handler(sig);
				}
			}

			// re-raise to trigger default handler (and core dump, if enabled)
			signal(sig, SIG_DFL);
			raise(sig);
		}

		void installAltStack()
		{
			static constexpr size_t kAltStackSz = 128 * 1024; // bigger than SIGSTKSZ for safety
			altStackMem = mmap(nullptr, kAltStackSz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
			if (altStackMem == MAP_FAILED)
				altStackMem = std::malloc(kAltStackSz); // fallback to malloc
			stack_t ss{};
			ss.ss_sp = altStackMem;
			ss.ss_size = kAltStackSz;
			ss.ss_flags = 0;
			if (sigaltstack(&ss, nullptr) != 0)
			{
				CAGE_LOG(SeverityEnum::Info, "crash-handler", strerror(errno));
				CAGE_THROW_ERROR(Exception, "sigaltstack")
			}
		}

		void installHandler(int sig)
		{
			struct sigaction sa
			{};
			sa.sa_sigaction = &crashHandler;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
			if (sigaction(sig, &sa, prevHandler(sig)) != 0)
			{
				CAGE_LOG(SeverityEnum::Info, "crash-handler", strerror(errno));
				CAGE_THROW_ERROR(Exception, "sigaction")
			}
		}

		struct SetupHandlers
		{
			SetupHandlers()
			{
				installAltStack();
				int signals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTRAP };
				for (int s : signals)
					installHandler(s);
			}
		} setupHandlers;
	}
}

#endif // CAGE_SYSTEM_LINUX
