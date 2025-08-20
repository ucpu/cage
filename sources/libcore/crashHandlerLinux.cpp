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
	String currentThreadName();

	int crashHandlerLogFileFd = STDERR_FILENO;

	void crashHandlerInstallAltStack()
	{
		static constexpr size_t AltStackSize = 128 * 1024; // bigger than SIGSTKSZ for safety

		{
			stack_t ss = {};
			if (sigaltstack(nullptr, &ss) == 0)
			{
				if ((ss.ss_flags & SS_DISABLE) == 0 && ss.ss_size >= AltStackSize)
					return; // already installed and sufficient
			}
		}

		void *altStackMem = mmap(nullptr, AltStackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
		if (altStackMem == MAP_FAILED)
			altStackMem = std::malloc(AltStackSize); // fallback to malloc

		stack_t ss{};
		ss.ss_sp = altStackMem;
		ss.ss_size = AltStackSize;
		ss.ss_flags = 0;
		if (sigaltstack(&ss, nullptr) != 0)
		{
			CAGE_LOG(SeverityEnum::Note, "crash-handler", strerror(errno));
			CAGE_LOG(SeverityEnum::Warning, "crash-handler", "failed to install alt stack");
		}
	}

	namespace
	{
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
			if (crashHandlerLogFileFd != STDERR_FILENO)
				write(STDERR_FILENO, s.c_str(), s.length());
		}

		void printStackTrace()
		{
			// backtrace (best-effort; not strictly async-signal-safe)
			static constexpr int MaxFrames = 256;
			void *frames[MaxFrames];
			int n = backtrace(frames, MaxFrames);
			safeWrite(Stringizer() + "stack trace:\n");
			backtrace_symbols_fd(frames, n, crashHandlerLogFileFd);
			if (crashHandlerLogFileFd != STDERR_FILENO)
				backtrace_symbols_fd(frames, n, STDERR_FILENO);
			safeWrite("\n");
		}

		void crashHandler(int sig, siginfo_t *si, void *uctx)
		{
			safeWrite(Stringizer() + "signal handler: " + sigToStr(sig) + " (" + sig + ")\n");

			if (si)
				safeWrite(Stringizer() + "fault addr: " + (uintptr_t)si->si_addr + ", code: " + si->si_code + "\n");

			safeWrite(Stringizer() + "in thread: " + currentThreadName() + "\n");
			printStackTrace();

			if (struct sigaction *prev = prevHandler(sig))
			{
				safeWrite("calling previous handler\n");
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
			else
			{
				safeWrite("calling default handler\n");
				signal(sig, SIG_DFL);
				raise(sig);
			}
		}

		void installHandler(int sig)
		{
			struct sigaction sa = {};
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
				crashHandlerInstallAltStack();
				int signals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTRAP };
				for (int s : signals)
					installHandler(s);
			}
		} setupHandlers;
	}
}

#endif // CAGE_SYSTEM_LINUX
