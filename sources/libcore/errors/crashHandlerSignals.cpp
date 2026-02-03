#if defined(CAGE_SYSTEM_LINUX) || defined(CAGE_SYSTEM_MAC)

	#include <cerrno>
	#include <csignal>
	#include <cstddef>
	#include <cstdio>
	#include <cstdlib>
	#include <cstring>
	#include <execinfo.h>
	#include <signal.h>
	#include <sys/mman.h>
	#include <sys/types.h>
	#include <unistd.h>

	#include <cage-core/process.h> // installSigTermHandler

namespace cage
{
	String currentThreadName();

	// this is updated when the logging system is initialized
	int crashHandlerLogFileFd = STDERR_FILENO;

	void crashHandlerSafeWrite(const String &s)
	{
		write(crashHandlerLogFileFd, s.c_str(), s.length());
		fsync(crashHandlerLogFileFd);
		if (crashHandlerLogFileFd != STDERR_FILENO)
		{
			write(STDERR_FILENO, s.c_str(), s.length());
			fsync(STDERR_FILENO);
		}
	}

	void crashHandlerThreadInit()
	{
		// install alternative stack memory for handling signals

		static constexpr size_t AltStackSize = 128 * 1024; // bigger than SIGSTKSZ for safety

		{
			stack_t ss = {};
			if (sigaltstack(nullptr, &ss) == 0)
			{
				if ((ss.ss_flags & SS_DISABLE) == 0 && ss.ss_size >= AltStackSize)
					return; // already installed and sufficient
			}
		}

		void *altStackMem = nullptr;
	#ifdef CAGE_SYSTEM_LINUX
		altStackMem = mmap(nullptr, AltStackSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	#endif
		if (altStackMem == nullptr || altStackMem == MAP_FAILED)
			altStackMem = std::malloc(AltStackSize); // fallback to malloc

		stack_t ss{};
		ss.ss_sp = altStackMem;
		ss.ss_size = AltStackSize;
		ss.ss_flags = 0;
		if (sigaltstack(&ss, nullptr) != 0)
		{
			CAGE_LOG(SeverityEnum::Note, "crash-handler", strerror(errno));
			CAGE_LOG(SeverityEnum::Warning, "crash-handler", "failed to install thread alt stack");
			// most signals can be handled on regular stack memory, so we continue
		}
	}

	namespace
	{
		constexpr int PrevHandlersCount = 16;
		struct sigaction prevHandlers[PrevHandlersCount] = {};

		struct sigaction *prevHandler(int sig)
		{
			if (sig >= 0 && sig < PrevHandlersCount)
				return &prevHandlers[sig];
			return nullptr;
		}
		const char *sigToStr(int sig)
		{
			switch (sig)
			{
				case SIGABRT:
					return "SIGABRT";
	#ifdef SIGALRM
				case SIGALRM:
					return "SIGALRM";
	#endif
				case SIGBUS:
					return "SIGBUS";
	#ifdef SIGCHLD
				case SIGCHLD:
					return "SIGCHLD";
	#endif
					// #ifdef SIGCLD
					// 			case SIGCLD:
					// 				return "SIGCLD"; // duplicate of SIGCHLD
					// #endif
				case SIGCONT:
					return "SIGCONT";
	#ifdef SIGEMT
				case SIGEMT:
					return "SIGEMT";
	#endif
				case SIGFPE:
					return "SIGFPE";
				case SIGHUP:
					return "SIGHUP";
				case SIGILL:
					return "SIGILL";
	#ifdef SIGINFO
				case SIGINFO:
					return "SIGINFO";
	#endif
				case SIGINT:
					return "SIGINT";
	#ifdef SIGIO
				case SIGIO:
					return "SIGIO";
	#endif
					// #ifdef SIGIOT
					// 			case SIGIOT:
					// 				return "SIGIOT"; // duplicate of SIGABRT
					// #endif
				case SIGKILL:
					return "SIGKILL";
	#ifdef SIGLOST
				case SIGLOST:
					return "SIGLOST";
	#endif
				case SIGPIPE:
					return "SIGPIPE";
					// #ifdef SIGPOLL
					// 			case SIGPOLL:
					// 				return "SIGPOLL"; // duplicate of SIGIO
					// #endif
				case SIGPROF:
					return "SIGPROF";
	#ifdef SIGPWR
				case SIGPWR:
					return "SIGPWR";
	#endif
				case SIGQUIT:
					return "SIGQUIT";
				case SIGSEGV:
					return "SIGSEGV";
	#ifdef SIGSTKFLT
				case SIGSTKFLT:
					return "SIGSTKFLT";
	#endif
				case SIGSTOP:
					return "SIGSTOP";
				case SIGSYS:
					return "SIGSYS";
				case SIGTERM:
					return "SIGTERM";
				case SIGTRAP:
					return "SIGTRAP";
				case SIGTSTP:
					return "SIGTSTP";
				case SIGTTIN:
					return "SIGTTIN";
				case SIGTTOU:
					return "SIGTTOU";
	#ifdef SIGUNUSED
				case SIGUNUSED:
					return "SIGUNUSED";
	#endif
				case SIGURG:
					return "SIGURG";
				case SIGUSR1:
					return "SIGUSR1";
				case SIGUSR2:
					return "SIGUSR2";
				case SIGVTALRM:
					return "SIGVTALRM";
				case SIGWINCH:
					return "SIGWINCH";
				case SIGXCPU:
					return "SIGXCPU";
				case SIGXFSZ:
					return "SIGXFSZ";
				default:
					return "UNKNOWN";
			}
		}

		void printStackTrace()
		{
			// backtrace (best-effort; not strictly async-signal-safe)
			static constexpr int MaxFrames = 256;
			void *frames[MaxFrames];
			int n = backtrace(frames, MaxFrames);
			crashHandlerSafeWrite(Stringizer() + "stack trace:\n");
			backtrace_symbols_fd(frames, n, crashHandlerLogFileFd);
			if (crashHandlerLogFileFd != STDERR_FILENO)
				backtrace_symbols_fd(frames, n, STDERR_FILENO);
			crashHandlerSafeWrite("\n");
		}

		void performPreviousHandler(int sig, siginfo_t *si, void *uctx)
		{
			if (struct sigaction *prev = prevHandler(sig))
			{
				crashHandlerSafeWrite("calling previous signal handler\n");
				if (prev->sa_flags & SA_SIGINFO)
					prev->sa_sigaction(sig, si, uctx);
				else if (prev->sa_handler == SIG_DFL || prev->sa_handler == SIG_IGN)
				{
					signal(sig, prev->sa_handler);
					raise(sig);
				}
				else
					prev->sa_handler(sig);
			}
			else
			{
				crashHandlerSafeWrite("calling default signal handler\n");
				signal(sig, SIG_DFL);
				raise(sig);
			}
		}

		void crashHandler(int sig, siginfo_t *si, void *uctx)
		{
			crashHandlerSafeWrite(Stringizer() + "signal handler: " + sigToStr(sig) + " (" + sig + ")\n");
			if (si)
				crashHandlerSafeWrite(Stringizer() + "fault addr: " + (uintptr_t)si->si_addr + ", code: " + si->si_code + "\n");
			crashHandlerSafeWrite(Stringizer() + "in thread: " + currentThreadName() + "\n");
			printStackTrace();
			performPreviousHandler(sig, si, uctx);
		}

		Delegate<void()> sigTermHandler;
		Delegate<void()> sigIntHandler;

		void intHandler(int sig, siginfo_t *si, void *uctx)
		{
			crashHandlerSafeWrite(Stringizer() + "signal handler: " + sigToStr(sig) + " (" + sig + ")\n");

			switch (sig)
			{
				case SIGTERM:
				{
					if (sigTermHandler)
					{
						sigTermHandler();
						return; // already handled, do not call default handler
					}
					break;
				}
				case SIGINT:
				{
					if (sigIntHandler)
					{
						sigIntHandler();
						return; // already handled, do not call default handler
					}
					break;
				}
			}

			performPreviousHandler(sig, si, uctx);
		}

		template<auto Handler>
		void installHandler(int sig)
		{
			struct sigaction sa = {};
			sa.sa_sigaction = Handler;
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
			if (sigaction(sig, &sa, prevHandler(sig)) != 0)
			{
				CAGE_LOG(SeverityEnum::Info, "crash-handler", strerror(errno));
				CAGE_THROW_ERROR(Exception, "sigaction")
			}
		}

		struct SetupHandlersSignals
		{
			SetupHandlersSignals()
			{
				crashHandlerThreadInit();
				// https://man7.org/linux/man-pages/man7/signal.7.html
				for (int s : {
						 SIGABRT, // Abort signal from abort()
						 // SIGALRM, // Timer signal from alarm()
						 SIGBUS, // Bus error (bad memory access)
						 // SIGCHLD, // Child stopped, terminated, or continued
						 // SIGCLD, // A synonym for SIGCHLD
						 // SIGCONT, // Continue if stopped
						 // SIGEMT, // Emulator trap
						 SIGFPE, // Erroneous arithmetic operation
						 // SIGHUP, // Hangup detected on controlling terminal or death of controlling process
						 SIGILL, // Illegal Instruction
						 // SIGINFO, // A synonym for SIGPWR
						 // SIGIO, // I/O now possible
						 // SIGIOT, // IOT trap.  A synonym for SIGABRT
						 // SIGKILL, // Kill signal
						 // SIGLOST, // File lock lost
						 // SIGPIPE, // Broken pipe
						 // SIGPOLL, // Pollable event
						 // SIGPROF, // Profiling timer expired
						 // SIGPWR, // Power failure
						 // SIGQUIT, // Quit from keyboard
						 SIGSEGV, // Invalid memory reference
						 // SIGSTKFLT, // Stack fault on coprocessor
						 // SIGSTOP, // Stop process
						 // SIGTSTP, // Stop typed at terminal
						 SIGSYS, // Bad system call
						 SIGTRAP, // Trace/breakpoint trap
						 // SIGTTIN, // Terminal input for background process
						 // SIGTTOU, // Terminal output for background process
						 // SIGUNUSED, // Synonymous with SIGSYS
						 // SIGURG, // Urgent condition on socket
						 // SIGUSR1, // User-defined signal 1
						 // SIGUSR2, // User-defined signal 2
						 // SIGVTALRM, // Virtual alarm clock
						 SIGXCPU, // CPU time limit exceeded
						 SIGXFSZ, // File size limit exceeded
						 // SIGWINCH, // Window resize signal
					 })
				{
					installHandler<&crashHandler>(s);
				}
				for (int s : {
						 SIGINT, // Interrupt from keyboard
						 SIGTERM, // Termination signal
					 })
				{
					installHandler<&intHandler>(s);
				}
			}
		} setupHandlersSignals;
	}

	void installSigTermHandler(Delegate<void()> handler)
	{
		sigTermHandler = handler;
	}

	void installSigIntHandler(Delegate<void()> handler)
	{
		sigIntHandler = handler;
	}
}

#endif // CAGE_SYSTEM_LINUX || CAGE_SYSTEM_MAC
