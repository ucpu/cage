#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h>
#include <cage-core/timer.h>
#include <cage-core/config.h>
#include <cage-core/systemInformation.h>

namespace cage
{
	namespace
	{
		configBool confDetailedInfo("cage-core.log.detailedVendorInfo", false);

		mutexClass *loggerMutex()
		{
			static holder<mutexClass> *m = new holder<mutexClass>(newMutex()); // this leak is intentional
			return m->get();
		}

		timerClass *loggerTimer()
		{
			static holder<timerClass> *t = new holder<timerClass>(newTimer()); // this leak is intentional
			return t->get();
		}

		class loggerImpl *&loggerLast()
		{
			static class loggerImpl *l = nullptr;
			return l;
		}

		class loggerImpl : public loggerClass
		{
		public:
			loggerImpl *prev, *next;
			const uint64 thread;

			loggerImpl() : prev(nullptr), next(nullptr), thread(threadId())
			{
				{
					scopeLock<mutexClass> l(loggerMutex());
					if (loggerLast())
					{
						prev = loggerLast();
						loggerLast()->next = this;
					}
					loggerLast() = this;
				}

				output.bind<&logOutputPolicyStdErr>();
			}

			~loggerImpl()
			{
				{
					scopeLock<mutexClass> l(loggerMutex());
					if (prev)
						prev->next = next;
					if (next)
						next->prev = prev;
					if (loggerLast() == this)
						loggerLast() = prev;
				}
			}
		};

		bool logFilterPolicyNoDebug(const detail::loggerInfo &info)
		{
			return !info.debug;
		}

		class centralLogClass
		{
		public:
			centralLogClass()
			{
				loggerDebug = newLogger();
				loggerDebug->filter.bind<&logFilterPolicyNoDebug>();
				loggerDebug->format.bind<&logFormatPolicyConsole>();
				loggerDebug->output.bind<&logOutputPolicyDebug>();

				loggerOutputCentralFile = newLogOutputPolicyFile(pathExtractFilename(detail::getExecutableFullPathNoExe()) + ".log", false);
				loggerCentralFile = newLogger();
				loggerCentralFile->output.bind<logOutputPolicyFileClass, &logOutputPolicyFileClass::output>(loggerOutputCentralFile.get());
				loggerCentralFile->format.bind<&logFormatPolicyFileShort>();
			}

			holder<loggerClass> loggerDebug;
			holder<logOutputPolicyFileClass> loggerOutputCentralFile;
			holder<loggerClass> loggerCentralFile;
		};

		class centralLogStaticInitializerClass
		{
		public:
			centralLogStaticInitializerClass()
			{
				{
					string version;

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
					version += "build at: " __DATE__ " " __TIME__;

					CAGE_LOG(severityEnum::Info, "log", string() + "cage version: " + version);
				}

				setCurrentThreadName(pathExtractFilename(detail::getExecutableFullPathNoExe()));

				CAGE_LOG(severityEnum::Info, "log", string() + "process id: " + processId());

				uint32 y, M, d, h, m, s;
				detail::getSystemDateTime(y, M, d, h, m, s);
				CAGE_LOG(severityEnum::Info, "log", string() + "current time: " + detail::formatDateTime(y, M, d, h, m, s));

				if (confDetailedInfo)
				{
					CAGE_LOG(severityEnum::Info, "log", "detailed system information:");
					try
					{
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "system name: '" + systemName() + "'");
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "user name: '" + userName() + "'");
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "host name: '" + hostName() + "'");
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "processors count: " + processorsCount());
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "processor name: '" + processorName() + "'");
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "processor speed: " + (processorClockSpeed() / 1000000) + " MHz");
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "memory capacity: " + (memoryCapacity() / 1024 / 1024) + " MB");
						CAGE_LOG_CONTINUE(severityEnum::Info, "systemInfo", string() + "memory available: " + (memoryAvailable() / 1024 / 1024) + " MB");
					}
					catch (exception &)
					{
						// do nothing
					}
				}
			}

			~centralLogStaticInitializerClass()
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
				CAGE_LOG(severityEnum::Info, "log", string() + "total duration: " + duration + " days, " + hours + " hours, " + mins + " minutes, " + secs + " seconds and " + micros + " microseconds");
			}
		} centralLogStaticInitializerInstance;
	}

	holder<loggerClass> newLogger()
	{
		return detail::systemArena().createImpl<loggerClass, loggerImpl>();
	}

	namespace detail
	{
		loggerInfo::loggerInfo() : component(""), file(nullptr), function(nullptr), time(0), createThreadId(0), currentThreadId(0), severity(severityEnum::Critical), line(0), continuous(false), debug(false)
		{}

		uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept
		{
			try
			{
				getCentralLog();
				loggerInfo info;
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

				scopeLock<mutexClass> l(loggerMutex());
				loggerImpl *cur = loggerLast();
				while (cur)
				{
					if (cur->output)
					{
						info.createThreadId = cur->thread;
						if (!cur->filter || cur->filter(info))
						{
							if (cur->format)
								cur->format(info, cur->output);
							else
								cur->output(info.message);
						}
					}
					cur = cur->prev;
				}

				return info.time;
			}
			catch (...)
			{
				debugOutput("makeLog exception");
			}
			return 0;
		}

		loggerClass *getCentralLog()
		{
			static centralLogClass *centralLogInstance = new centralLogClass(); // this leak is intentional
			return centralLogInstance->loggerCentralFile.get();
		}
	}

	uint64 getApplicationTime()
	{
		return loggerTimer()->microsSinceStart();
	}
}

