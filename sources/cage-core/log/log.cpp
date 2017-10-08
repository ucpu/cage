#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/timer.h>

namespace cage
{
	namespace
	{
		mutexClass *loggerMutex()
		{
			static holder<mutexClass> *m = new holder<mutexClass>(newMutex()); // this leak is intentionall
			return m->get();
		}

		timerClass *loggerTimer()
		{
			static holder<timerClass> *t = new holder<timerClass>(newTimer()); // this leak is intentionall
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
		
		class logInitializerClass
		{
		public:
			logInitializerClass()
			{
#ifdef _MSC_VER
				loggerDebug = newLogger();
				loggerDebug->output.bind<&logOutputPolicyDebug>();
				loggerDebug->format.bind<&logFormatPolicyDebug>();
#endif

				loggerOutputCentralFile = newLogOutputPolicyFile(pathExtractFilenameNoExtension(detail::getExecutableName()) + ".log", false);
				loggerCentralFile = newLogger();
				loggerCentralFile->output.bind<logOutputPolicyFileClass, &logOutputPolicyFileClass::output>(loggerOutputCentralFile.get());
				loggerCentralFile->format.bind<&logFormatPolicyFile>();

				CAGE_LOG(severityEnum::Info, "log", "application log initialized");

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
#else
	#error unknown platform
#endif // CAGE_SYSTEM_WINDOWS
					version += ", ";
	
#ifdef CAGE_ARCHITECTURE_64
					version += "64";
#else
					version += "32";
#endif // CAGE_ARCHITECTURE_64

					CAGE_LOG(severityEnum::Info, "log", string() + "cage version: " + version + " bit, build at: " __DATE__ " " __TIME__);
				}

				CAGE_LOG(severityEnum::Info, "log", string() + "process id: " + processId());

				uint32 y, M, d, h, m, s;
				getSystemDateTime(y, M, d, h, m, s);
				CAGE_LOG(severityEnum::Info, "log", string() + "current time: " + formatDateTime(y, M, d, h, m, s));
			}

			~logInitializerClass()
			{
				uint64 duration = CAGE_LOG(severityEnum::Info, "log", "application log finalized");
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

			holder<loggerClass> loggerDebug;
			holder<logOutputPolicyFileClass> loggerOutputCentralFile;
			holder<loggerClass> loggerCentralFile;
		} logInitializerInstance;
	}

	holder<loggerClass> newLogger()
	{
		return detail::systemArena().createImpl<loggerClass, loggerImpl>();
	}

	namespace detail
	{
#ifdef CAGE_DEBUG
		uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const string &component, const string &message, bool continuous) noexcept
#else
		uint64 makeLog(severityEnum severity, const string &component, const string &message, bool continuous) noexcept
#endif
		{
			try
			{
				loggerInfo info;
				info.severity = severity;
				info.component = component;
				info.message = message;
				info.continuous = continuous;
				info.currentThread = threadId();
				info.time = getApplicationTime();
#ifdef CAGE_DEBUG
				info.file = file;
				info.line = line;
				info.function = function;
#endif

				scopeLock<mutexClass> l(loggerMutex());
				loggerImpl *cur = loggerLast();
				while (cur)
				{
					info.createThread = cur->thread;
					if (cur->output)
					{
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
			return logInitializerInstance.loggerCentralFile.get();
		}
	}

	uint64 getApplicationTime()
	{
		if (loggerTimer())
			return loggerTimer()->microsSinceStart();
		else
			return 0;
	}
}

