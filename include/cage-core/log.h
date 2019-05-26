#ifndef guard_log_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
#define guard_log_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_

namespace cage
{
	namespace detail
	{
		struct CAGE_API loggerInfo
		{
			string message;
			string currentThreadName;
			const char *component;
			const char *file;
			const char *function;
			uint64 time;
			uint64 createThreadId;
			uint64 currentThreadId;
			uint32 line;
			severityEnum severity;
			bool continuous;
			bool debug;
			loggerInfo();
		};
	}

	class CAGE_API loggerClass
	{
	public:
		delegate<bool(const detail::loggerInfo &)> filter;
		delegate<void(const detail::loggerInfo &, delegate<void(const string &)>)> format;
		delegate<void(const string &)> output;
	};

	CAGE_API holder<loggerClass> newLogger();

	CAGE_API void logFormatPolicyConsole(const detail::loggerInfo &info, delegate<void(const string &)> output);
	CAGE_API void logFormatPolicyFileShort(const detail::loggerInfo &info, delegate<void(const string &)> output);
	CAGE_API void logFormatPolicyFileLong(const detail::loggerInfo &info, delegate<void(const string &)> output);

	CAGE_API void logOutputPolicyDebug(const string &message);
	CAGE_API void logOutputPolicyStdOut(const string &message);
	CAGE_API void logOutputPolicyStdErr(const string &message);

	class CAGE_API logOutputPolicyFileClass
	{
	public:
		void output(const string &message);
	};

	CAGE_API holder<logOutputPolicyFileClass> newLogOutputPolicyFile(const string &path, bool append);

	namespace detail
	{
		CAGE_API loggerClass *getCentralLog();
		CAGE_API string severityToString(const severityEnum severity);
	}

	namespace privat
	{
		CAGE_API uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept;
		inline uint64 makeLog(severityEnum severity, const char *component, const string &message, bool continuous, bool debug) { return makeLog(nullptr, 0, nullptr, severity, component, message, continuous, debug); }
	}
}

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, true)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::privat::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, true)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#endif

#endif // guard_log_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
