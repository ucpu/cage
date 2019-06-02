#include <cstdio>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
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

				{
					uint32 y, M, d, h, m, s;
					detail::getSystemDateTime(y, M, d, h, m, s);
					CAGE_LOG(severityEnum::Info, "log", string() + "current time: " + detail::formatDateTime(y, M, d, h, m, s));
				}

				CAGE_LOG(severityEnum::Info, "log", string() + "executable path: " + detail::getExecutableFullPath());
				CAGE_LOG(severityEnum::Info, "log", string() + "working directory: " + pathWorkingDir());

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

		void logFormatPolicyFileImpl(const detail::loggerInfo &info, delegate<void(const string &)> output, bool longer)
		{
			if (info.continuous)
			{
				output(string("\t") + info.message);
			}
			else
			{
				string res;
				res += string(info.time).fill(12) + " ";
				res += string(info.currentThreadName).fill(26) + " ";
				res += detail::severityToString(info.severity) + " ";
				res += string(info.component).fill(20) + " ";
				res += info.message;
				if (longer && info.file)
				{
					string flf = string(" ") + string(info.file) + ":" + string(info.line) + " (" + string(info.function) + ")";
					if (res.length() + flf.length() + 10 < string::MaxLength)
					{
						res += string().fill(string::MaxLength - flf.length() - res.length() - 5);
						res += flf;
					}
				}
				output(res);
			}
		}

		class fileOutputPolicyImpl : public logOutputPolicyFileClass
		{
		public:
			fileOutputPolicyImpl(const string &path, bool append)
			{
				fileMode fm(false, true);
				fm.textual = true;
				fm.append = append;
				f = newFile(path, fm);
			}

			holder<fileClass> f;
		};
	}

	holder<loggerClass> newLogger()
	{
		return detail::systemArena().createImpl<loggerClass, loggerImpl>();
	}

	namespace detail
	{
		loggerInfo::loggerInfo() : component(""), file(nullptr), function(nullptr), time(0), createThreadId(0), currentThreadId(0), severity(severityEnum::Critical), line(0), continuous(false), debug(false)
		{}

		loggerClass *getCentralLog()
		{
			static centralLogClass *centralLogInstance = new centralLogClass(); // this leak is intentional
			return centralLogInstance->loggerCentralFile.get();
		}

		string severityToString(severityEnum severity)
		{
			switch (severity)
			{
			case severityEnum::Hint: return "hint";
			case severityEnum::Note: return "note";
			case severityEnum::Info: return "info";
			case severityEnum::Warning: return "warn";
			case severityEnum::Error: return "EROR";
			case severityEnum::Critical: return "CRIT";
			default: return "UNKN";
			}
		}
	}

	namespace privat
	{
		uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept
		{
			try
			{
				detail::getCentralLog();
				detail::loggerInfo info;
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
				detail::debugOutput("makeLog exception");
			}
			return 0;
		}
	}

	void logFormatPolicyConsole(const detail::loggerInfo &info, delegate<void(const string &)> output)
	{
		output(info.message);
	}

	void logFormatPolicyFileShort(const detail::loggerInfo &info, delegate<void(const string &)> output)
	{
		logFormatPolicyFileImpl(info, output, false);
	}

	void logFormatPolicyFileLong(const detail::loggerInfo &info, delegate<void(const string &)> output)
	{
		logFormatPolicyFileImpl(info, output, true);
	}

	void logOutputPolicyDebug(const string &message)
	{
		detail::debugOutput(message.c_str());
	}

	void logOutputPolicyStdOut(const string &message)
	{
		fprintf(stdout, "%s\n", message.c_str());
		fflush(stdout);
	}

	void logOutputPolicyStdErr(const string &message)
	{
		fprintf(stderr, "%s\n", message.c_str());
		fflush(stderr);
	}

	void logOutputPolicyFileClass::output(const string &message)
	{
		fileOutputPolicyImpl *impl = (fileOutputPolicyImpl*)this;
		impl->f->writeLine(message);
		impl->f->flush();
	}

	holder<logOutputPolicyFileClass> newLogOutputPolicyFile(const string &path, bool append)
	{
		return detail::systemArena().createImpl<logOutputPolicyFileClass, fileOutputPolicyImpl>(path, append);
	}

	uint64 getApplicationTime()
	{
		return loggerTimer()->microsSinceStart();
	}
}
