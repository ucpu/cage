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
			const uint64 thread = threadId();

			LoggerImpl()
			{
				{
					ScopeLock l(loggerMutex());
					if (loggerLast())
					{
						prev = loggerLast();
						loggerLast()->next = this;
					}
					loggerLast() = this;
				}

				output.bind<&logOutputStdErr>();
			}

			~LoggerImpl()
			{
				{
					ScopeLock l(loggerMutex());
					if (prev)
						prev->next = next;
					if (next)
						next->prev = prev;
					if (loggerLast() == this)
						loggerLast() = prev;
				}
			}
		};

		class ApplicationLog
		{
		public:
			ApplicationLog()
			{
				if (detail::isDebugging())
				{
					loggerDebug = newLogger();
					loggerDebug->format.bind<&logFormatConsole>();
					loggerDebug->output.bind<&logOutputDebug>();
				}

				loggerOutputFile = newLoggerOutputFile(pathExtractFilename(detail::getExecutableFullPathNoExe()) + ".log", false, true);
				loggerFile = newLogger();
				loggerFile->output.bind<LoggerOutputFile, &LoggerOutputFile::output>(loggerOutputFile.get());
				loggerFile->format.bind<&logFormatFileShort>();
			}

			Holder<Logger> loggerDebug;
			Holder<LoggerOutputFile> loggerOutputFile;
			Holder<Logger> loggerFile;
		};

		class ApplicationLogInitializer
		{
		public:
			ApplicationLogInitializer()
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

				CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "process id: " + processId());

				{
					const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
					char buffer[50];
					std::strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
					CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "current time: " + buffer);
				}

				CAGE_LOG(SeverityEnum::Info, "log", stringizer() + "executable path: " + detail::getExecutableFullPath());
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

				setCurrentThreadName(pathExtractFilename(detail::getExecutableFullPathNoExe()));
			}

			~ApplicationLogInitializer()
			{
				uint64 duration = getApplicationTime();
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
		} applicationLogInitializerInstance;

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
				res += fill(string(info.component), 20) + " ";
				res += info.message;
				if (longer && info.file)
				{
					string flf = stringizer() + " " + info.file + ":" + info.line + " (" + info.function + ")";
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

			Holder<File> f;
		};
	}

	Holder<Logger> newLogger()
	{
		return detail::systemArena().createImpl<Logger, LoggerImpl>();
	}

	namespace detail
	{
		Logger *getApplicationLog()
		{
			static ApplicationLog *centralLogInstance = new ApplicationLog(); // this leak is intentional
			return +centralLogInstance->loggerFile;
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
				CAGE_LOG(SeverityEnum::Info, "exception", stringizer() + "cage exception: " + e.message);
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
		uint64 makeLog(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept
		{
			try
			{
				detail::getApplicationLog(); // ensure application logger was initialized

				detail::LoggerInfo info;
				info.message = message;
				info.component = component;
				info.severity = severity;
				info.continuous = continuous;
				info.debug = debug;
				info.currentThreadId = threadId();
				info.currentThreadName = getCurrentThreadName();
				info.time = getApplicationTime();
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

	void LoggerOutputFile::output(const string &message)
	{
		LoggerOutputFileImpl *impl = (LoggerOutputFileImpl*)this;
		impl->f->writeLine(message);
		realTryFlushFile(+impl->f);
	}

	Holder<LoggerOutputFile> newLoggerOutputFile(const string &path, bool append, bool realFilesystemOnly)
	{
		return detail::systemArena().createImpl<LoggerOutputFile, LoggerOutputFileImpl>(path, append, realFilesystemOnly);
	}

	uint64 getApplicationTime()
	{
		return loggerTimer()->microsSinceStart();
	}
}
