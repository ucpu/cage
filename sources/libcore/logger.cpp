#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>

#ifdef CAGE_SYSTEM_WINDOWS
	#include "windowsMinimumInclude.h" // SetConsoleCP
#else
	#include <unistd.h> // STDERR_FILENO
#endif

#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/files.h>
#include <cage-core/logger.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/string.h>
#include <cage-core/systemInformation.h>
#include <cage-core/timer.h>

namespace cage
{
	namespace privat
	{
#ifdef CAGE_SYSTEM_WINDOWS
		int crashHandlerLogFileFd = _fileno(stderr);
#else
		int crashHandlerLogFileFd = STDERR_FILENO;
#endif // CAGE_SYSTEM_WINDOWS

		int realFileGetFd(File *f);
	}

	namespace
	{
		const ConfigBool confDetailedInfo("cage/systemInfo/autoLog", false);

		Mutex *loggerMutex()
		{
			static Holder<Mutex> *m = new Holder<Mutex>(newMutex()); // this leak is intentional
			return m->get();
		}

		Timer *loggerTimer()
		{
			static Holder<Timer> *t = new Holder<Timer>(newTimer()); // this leak is intentional
			return t->get();
		}

		class LoggerImpl *&loggerLast()
		{
			static class LoggerImpl *l = nullptr;
			return l;
		}

		class LoggerImpl : public Logger
		{
		public:
			LoggerImpl *prev = nullptr, *next = nullptr;
			const uint64 thread = currentThreadId();

			LoggerImpl()
			{
				ScopeLock l(loggerMutex());
				if (loggerLast())
				{
					prev = loggerLast();
					loggerLast()->next = this;
				}
				loggerLast() = this;
				output.bind<logOutputStdErr>();
			}

			~LoggerImpl()
			{
				ScopeLock l(loggerMutex());
				if (prev)
					prev->next = next;
				if (next)
					next->prev = prev;
				if (loggerLast() == this)
					loggerLast() = prev;
			}
		};
	}

	Holder<Logger> newLogger()
	{
		return systemMemory().createImpl<Logger, LoggerImpl>();
	}

	StringPointer severityToString(SeverityEnum severity)
	{
		switch (severity)
		{
			case SeverityEnum::Hint:
				return "hint";
			case SeverityEnum::Note:
				return "note";
			case SeverityEnum::Info:
				return "info";
			case SeverityEnum::Warning:
				return "warn";
			case SeverityEnum::Error:
				return "EROR";
			case SeverityEnum::Critical:
				return "CRIT";
			default:
				return "UNKN";
		}
	}

	namespace privat
	{
		uint64 makeLog(const std::source_location &location, SeverityEnum severity, StringPointer component, PointerRange<const char> message, bool continuous, bool debug) noexcept
		{
			detail::globalLogger(); // ensure global logger was initialized

			try
			{
				CAGE_ASSERT(message.data());

				// trim trailing new lines
				while (true)
				{
					if (message.empty())
						break;
					const char c = message[message.size() - 1];
					switch (c)
					{
						case '\r':
						case '\n':
						case '\t':
						case ' ':
							message = message.subRange(0, message.size() - 1);
							continue;
					}
					break;
				}

				detail::LoggerInfo info;
				info.location = location;
				info.message = message;
				info.component = component;
				info.severity = severity;
				info.continuous = continuous;
				info.debug = debug;
				info.currentThreadId = currentThreadId();
				info.currentThreadName = currentThreadName();
				info.time = applicationTime();

				{
					ScopeLock l(loggerMutex());
					LoggerImpl *cur = loggerLast();
					while (cur)
					{
						const auto ou = cur->output; // keep a copy in case the configuration changed mid-way
						if (ou)
						{
							const auto fi = cur->filter;
							info.createThreadId = cur->thread;
							if (!fi || fi(info))
							{
								const auto fo = cur->format;
								if (fo)
									fo(info, ou);
								else
									ou(info.message);
							}
						}
						cur = cur->prev;
					}
				}

				return info.time;
			}
			catch (...)
			{
				detail::debugOutput("ignoring makeLog exception");
				detail::debugBreakpoint();
			}
			return 0;
		}
	}

	void logFormatConsole(const detail::LoggerInfo &info, Delegate<void(PointerRange<const char>)> output)
	{
		output(info.message);
	}

	namespace
	{
		std::string &operator+=(std::string &s, const String &v)
		{
			s.append(v.data(), v.size());
			return s;
		}
	}

	void logFormatFile(const detail::LoggerInfo &info, Delegate<void(PointerRange<const char>)> output)
	{
		static std::string res; // avoid unnecessary reallocations, logging is under mutex so this is safe
		res.clear();
		res.reserve(info.message.size() + 200);
		if (info.continuous)
		{
			res += "\t";
		}
		else
		{
			res += fill(String(Stringizer() + info.time), 12) + " ";
			res += fill(String(info.currentThreadName), 26) + " ";
			res += Stringizer() + severityToString(info.severity) + " ";
			res += fill(String(info.component), 20) + " ";
		}
		res.append(info.message.data(), info.message.size());
		output(res);
	}

	void logOutputDebug(PointerRange<const char> message)
	{
		detail::debugOutput(message);
	}

	void logOutputStdOut(PointerRange<const char> message)
	{
		fprintf(stdout, "%.*s\n", (int)message.size(), message.data());
		fflush(stdout);
	}

	void logOutputStdErr(PointerRange<const char> message)
	{
		fprintf(stderr, "%.*s\n", (int)message.size(), message.data());
		fflush(stderr);
	}

	namespace
	{
		class LoggerOutputFileImpl : public LoggerOutputFile
		{
		public:
			LoggerOutputFileImpl(const String &path, bool append, bool realFilesystemOnly)
			{
				FileMode fm(false, true);
				fm.textual = true;
				fm.append = append;
				if (realFilesystemOnly)
					f = detail::newRealFsFile(path, fm);
				else
					f = newFile(path, fm);
			}

			LoggerOutputFileImpl(Holder<File> file) : f(std::move(file)) { CAGE_ASSERT(f->mode().write); }

			Holder<File> f;
		};
	}

	void LoggerOutputFile::output(PointerRange<const char> message) const
	{
		const LoggerOutputFileImpl *impl = (const LoggerOutputFileImpl *)this;
		impl->f->write(message);
		impl->f->writeLine();
		detail::realFsAttemptFlush(+impl->f);
	}

	Holder<LoggerOutputFile> newLoggerOutputFile(const String &path, bool append, bool realFilesystemOnly)
	{
		return systemMemory().createImpl<LoggerOutputFile, LoggerOutputFileImpl>(path, append, realFilesystemOnly);
	}

	Holder<LoggerOutputFile> newLoggerOutputFile(Holder<File> file)
	{
		return systemMemory().createImpl<LoggerOutputFile, LoggerOutputFileImpl>(std::move(file));
	}

	uint64 applicationTime()
	{
		return loggerTimer()->duration();
	}

	namespace
	{
		String fullCommandLineImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			const char *p = GetCommandLine();
			auto l = detail::strlen(p);
			if (l > String::MaxLength / 2)
				l = String::MaxLength / 2;
			return String(PointerRange(p, p + l));
#elif defined(CAGE_SYSTEM_LINUX)
			FILE *fp = fopen("/proc/self/cmdline", "r");
			if (!fp)
				return "";
			auto _ = ScopeGuard([fp]() { fclose(fp); });
			String res;
			while (res.length() < String::MaxLength / 2)
			{
				char c = 0;
				auto l = fread(&c, 1, 1, fp);
				if (l <= 0)
					break;
				CAGE_ASSERT(l == 1);
				if (c == 0)
					c = ' '; // /proc/self/cmdline contains null terminators for each parameter
				res += String(c);
			}
			return res;
#elif defined(CAGE_SYSTEM_MAC)
			return ""; // todo
#else
	#error unknown platform
#endif // CAGE_SYSTEM_WINDOWS
		}

		class InitialLog : private Immovable
		{
		public:
			InitialLog()
			{
				{
					String version = "cage version: ";

#ifdef CAGE_DEBUG
					version += "debug";
#else
					version += "release";
#endif // CAGE_DEBUG
					version += ", ";

#ifdef CAGE_SYSTEM_WINDOWS
					version += "windows";
#elif defined(CAGE_SYSTEM_LINUX)
					version += "linux";
#elif defined(CAGE_SYSTEM_MAC)
					version += "mac";
#else
	#error unknown platform
#endif // CAGE_SYSTEM_WINDOWS
					version += ", ";

#ifdef CAGE_ARCHITECTURE_64
					version += "64 bit";
#else
					version += "32 bit";
#endif // CAGE_ARCHITECTURE_64
					version += ", ";
					version += "built at: " __DATE__ " " __TIME__;

					CAGE_LOG(SeverityEnum::Info, "log", version);
				}

				CAGE_LOG(SeverityEnum::Info, "log", Stringizer() + "process id: " + currentProcessId());

				{
					const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
					char buffer[100];
					std::strftime(buffer, sizeof(buffer), "%F %T (%z)", std::localtime(&now));
					CAGE_LOG(SeverityEnum::Info, "log", Stringizer() + "current time: " + buffer);
				}

				CAGE_LOG(SeverityEnum::Info, "log", Stringizer() + "command line: " + fullCommandLineImpl());
				CAGE_LOG(SeverityEnum::Info, "log", Stringizer() + "executable path: " + detail::pathExecutable());
				CAGE_LOG(SeverityEnum::Info, "log", Stringizer() + "working directory: " + detail::pathWorkingDir());

				if (confDetailedInfo)
				{
					CAGE_LOG(SeverityEnum::Info, "systemInfo", "system information:");
					try
					{
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "system name: " + systemName());
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "user name: " + userName());
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "host name: " + hostName());
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "processor name: " + processorName());
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "processor speed: " + (processorClockSpeed() / 1000000) + " MHz");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "threads count: " + processorsCount());
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "memory capacity: " + (memoryCapacity() / 1024 / 1024) + " MB");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", Stringizer() + "memory available: " + (memoryAvailable() / 1024 / 1024) + " MB");
					}
					catch (Exception &)
					{
						// do nothing
					}
				}

				currentThreadName(pathExtractFilename(detail::pathExecutableNoExe()));
			}

			~InitialLog()
			{
				uint64 duration = applicationTime();
				uint32 micros = numeric_cast<uint32>(duration % 1'000'000);
				duration /= 1'000'000;
				uint32 secs = numeric_cast<uint32>(duration % 60);
				duration /= 60;
				uint32 mins = numeric_cast<uint32>(duration % 60);
				duration /= 60;
				uint32 hours = numeric_cast<uint32>(duration % 24);
				duration /= 24;
				CAGE_LOG(SeverityEnum::Info, "log", Stringizer() + "total duration: " + duration + " days, " + hours + " hours, " + mins + " minutes, " + secs + " seconds and " + micros + " microseconds");
			}
		} initialLogInstance;

		struct ConsoleLogger : private Immovable
		{
			Holder<Logger> conLog = newLogger();

			ConsoleLogger()
			{
				conLog->format.bind<logFormatConsole>();
				conLog->output.bind<logOutputStdOut>();
			}
		};
	}

	Logger *initializeConsoleLogger()
	{
		static ConsoleLogger *inst = new ConsoleLogger(); // intentional leak
		return +inst->conLog;
	}

	namespace detail
	{
		Logger *globalLogger()
		{
			class GlobalLogger : private Immovable
			{
			public:
				GlobalLogger()
				{
#ifdef CAGE_SYSTEM_WINDOWS
					SetConsoleCP(CP_UTF8);
					SetConsoleOutputCP(CP_UTF8);
#endif

					if (detail::isDebugging())
					{
						loggerDebug = newLogger();
						loggerDebug->format.bind<logFormatConsole>();
						loggerDebug->output.bind<logOutputDebug>();
					}

					loggerFile = newLogger();
					loggerFile->format.bind<logFormatFile>();

					try
					{
						detail::OverrideException oe; // avoid deadlock when the file cannot be opened - the logger is still under construction
						loggerOutputFile = newLoggerOutputFile(pathExtractFilename(detail::pathExecutableNoExe()) + ".log", false, true);
						loggerFile->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(+loggerOutputFile);
						privat::crashHandlerLogFileFd = privat::realFileGetFd(+((LoggerOutputFileImpl *)+loggerOutputFile)->f);
					}
					catch (const cage::SystemError &)
					{
						// do nothing
					}
				}

				Holder<Logger> loggerDebug;
				Holder<LoggerOutputFile> loggerOutputFile;
				Holder<Logger> loggerFile;
			};

			static GlobalLogger *appLoggerInstance = new GlobalLogger(); // this leak is intentional
			return +appLoggerInstance->loggerFile;
		}

		void logCurrentCaughtException() noexcept
		{
			auto ex = std::current_exception();
			if (!ex)
			{
				CAGE_LOG(SeverityEnum::Warning, "exception", "trying to log an exception, but no exception is available");
				return;
			}
			try
			{
				std::rethrow_exception(ex);
			}
			catch (const cage::Exception &e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", Stringizer() + "cage exception: " + e.message);
			}
			catch (const std::exception &e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", Stringizer() + "std exception: " + e.what());
			}
			catch (const char *e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", Stringizer() + "c string exception: " + e);
			}
			catch (int e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", Stringizer() + "int exception: " + e);
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", "unknown exception type");
			}
		}
	}
}
