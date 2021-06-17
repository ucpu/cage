#include <cage-core/logger.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/timer.h>
#include <cage-core/config.h>
#include <cage-core/systemInformation.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>

#include <cstdio>
#include <exception>
#include <chrono>
#include <ctime>

namespace cage
{
	Holder<File> realNewFile(const string &path, const FileMode &mode);
	void realTryFlushFile(File *f);

	namespace
	{
		ConfigBool confDetailedInfo("cage/system/logVendorInfo", false);

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
				output.bind<&logOutputStdErr>();
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

		class GlobalLogger
		{
		public:
			GlobalLogger()
			{
				if (detail::isDebugging())
				{
					loggerDebug = newLogger();
					loggerDebug->format.bind<&logFormatConsole>();
					loggerDebug->output.bind<&logOutputDebug>();
				}

				loggerFile = newLogger();
				loggerFile->format.bind<&logFormatFileShort>();

				try
				{
					detail::OverrideException oe; // avoid deadlock when the file cannot be opened - the logger is still under construction
					loggerOutputFile = newLoggerOutputFile(pathExtractFilename(detail::executableFullPathNoExe()) + ".log", false, true);
					loggerFile->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(+loggerOutputFile);
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

		class GlobalLoggerInitializer
		{
		public:
			GlobalLoggerInitializer()
			{
				{
					string version = "cage version: ";

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

				CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "process id: " + currentProcessId());

				{
					const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
					char buffer[50];
					std::strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
					CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "current time: " + buffer);
				}

				CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "executable path: " + detail::executableFullPath());
				CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "working directory: " + pathWorkingDir());

				if (confDetailedInfo)
				{
					CAGE_LOG(SeverityEnum::Info, "systemInfo", "system information:");
					try
					{
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "system name: '" + systemName() + "'");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "user name: '" + userName() + "'");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "host name: '" + hostName() + "'");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "processors count: " + processorsCount());
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "processor name: '" + processorName() + "'");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "processor speed: " + (processorClockSpeed() / 1000000) + " MHz");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "memory capacity: " + (memoryCapacity() / 1024 / 1024) + " MB");
						CAGE_LOG_CONTINUE(SeverityEnum::Info, "systemInfo", stringizer() + "memory available: " + (memoryAvailable() / 1024 / 1024) + " MB");
					}
					catch (Exception &)
					{
						// do nothing
					}
				}

				currentThreadName(pathExtractFilename(detail::executableFullPathNoExe()));
			}

			~GlobalLoggerInitializer()
			{
				uint64 duration = applicationTime();
				uint32 micros = numeric_cast<uint32>(duration % 1000000);
				duration /= 1000000;
				uint32 secs = numeric_cast<uint32>(duration % 60);
				duration /= 60;
				uint32 mins = numeric_cast<uint32>(duration % 60);
				duration /= 60;
				uint32 hours = numeric_cast<uint32>(duration % 24);
				duration /= 24;
				CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "total duration: " + duration + " days, " + hours + " hours, " + mins + " minutes, " + secs + " seconds and " + micros + " microseconds");
			}
		} globalLoggerInitializerInstance;

		void logFormatFileImpl(const detail::LoggerInfo &info, Delegate<void(const string &)> output, bool longer)
		{
			if (info.continuous)
			{
				output(stringizer() + "\t" + info.message);
			}
			else
			{
				string res;
				res += fill(string(stringizer() + info.time), 12) + " ";
				res += fill(string(info.currentThreadName), 26) + " ";
				res += detail::severityToString(info.severity) + " ";
				res += fill(string(info.component.str), 20) + " ";
				res += info.message;
				if (longer && info.file.str)
				{
					string flf = stringizer() + " " + info.file.str + ":" + info.line + " (" + info.function.str + ")";
					if (res.length() + flf.length() + 10 < string::MaxLength)
					{
						res += fill(string(), string::MaxLength - flf.length() - res.length() - 5);
						res += flf;
					}
				}
				output(res);
			}
		}

		class LoggerOutputFileImpl : public LoggerOutputFile
		{
		public:
			LoggerOutputFileImpl(const string &path, bool append, bool realFilesystemOnly)
			{
				FileMode fm(false, true);
				fm.textual = true;
				fm.append = append;
				if (realFilesystemOnly)
					f = realNewFile(path, fm);
				else
					f = newFile(path, fm);
			}

			LoggerOutputFileImpl(Holder<File> file) : f(std::move(file))
			{
				CAGE_ASSERT(f->mode().write);
			}

			Holder<File> f;
		};
	}

	Holder<Logger> newLogger()
	{
		return systemMemory().createImpl<Logger, LoggerImpl>();
	}

	namespace detail
	{
		Logger *globalLogger()
		{
			static GlobalLogger *appLoggerInstance = new GlobalLogger(); // this leak is intentional
			return +appLoggerInstance->loggerFile;
		}

		string severityToString(SeverityEnum severity)
		{
			switch (severity)
			{
			case SeverityEnum::Hint: return "hint";
			case SeverityEnum::Note: return "note";
			case SeverityEnum::Info: return "info";
			case SeverityEnum::Warning: return "warn";
			case SeverityEnum::Error: return "EROR";
			case SeverityEnum::Critical: return "CRIT";
			default: return "UNKN";
			}
		}

		void logCurrentCaughtException()
		{
			try
			{
				throw;
			}
			catch (const cage::Exception &e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", stringizer() + "cage exception: " + e.message.str);
			}
			catch (const std::exception &e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", stringizer() + "std exception: " + e.what());
			}
			catch (const char *e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", stringizer() + "c string exception: " + e);
			}
			catch (int e)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", stringizer() + "int exception: " + e);
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Info, "exception", "unknown exception type");
			}
		}
	}

	namespace privat
	{
		uint64 makeLog(StringLiteral file, uint32 line, StringLiteral function, SeverityEnum severity, StringLiteral component, const string &message, bool continuous, bool debug) noexcept
		{
			detail::globalLogger(); // ensure global logger was initialized

			try
			{
				detail::LoggerInfo info;
				info.message = message;
				info.component = component;
				info.severity = severity;
				info.continuous = continuous;
				info.debug = debug;
				info.currentThreadId = currentThreadId();
				info.currentThreadName = currentThreadName();
				info.time = applicationTime();
				info.file = file;
				info.line = line;
				info.function = function;

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
			}
			return 0;
		}
	}

	void logFormatConsole(const detail::LoggerInfo &info, Delegate<void(const string &)> output)
	{
		output(info.message);
	}

	void logFormatFileShort(const detail::LoggerInfo &info, Delegate<void(const string &)> output)
	{
		logFormatFileImpl(info, output, false);
	}

	void logFormatFileLong(const detail::LoggerInfo &info, Delegate<void(const string &)> output)
	{
		logFormatFileImpl(info, output, true);
	}

	void logOutputDebug(const string &message)
	{
		detail::debugOutput(message.c_str());
	}

	void logOutputStdOut(const string &message)
	{
		fprintf(stdout, "%s\n", message.c_str());
		fflush(stdout);
	}

	void logOutputStdErr(const string &message)
	{
		fprintf(stderr, "%s\n", message.c_str());
		fflush(stderr);
	}

	void LoggerOutputFile::output(const string &message) const
	{
		const LoggerOutputFileImpl *impl = (const LoggerOutputFileImpl *)this;
		impl->f->writeLine(message);
		realTryFlushFile(+impl->f);
	}

	Holder<LoggerOutputFile> newLoggerOutputFile(const string &path, bool append, bool realFilesystemOnly)
	{
		return systemMemory().createImpl<LoggerOutputFile, LoggerOutputFileImpl>(path, append, realFilesystemOnly);
	}

	Holder<LoggerOutputFile> newLoggerOutputFile(Holder<File> file)
	{
		return systemMemory().createImpl<LoggerOutputFile, LoggerOutputFileImpl>(std::move(file));
	}

	uint64 applicationTime()
	{
		return loggerTimer()->microsSinceStart();
	}
}
