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

	namespace detail
	{
		CAGE_API loggerClass *getCentralLog();
		CAGE_API uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const char *component, const string &message, bool continuous, bool debug) noexcept;
		inline uint64 makeLog(severityEnum severity, const char *component, const string &message, bool continuous, bool debug) { return makeLog(nullptr, 0, nullptr, severity, component, message, continuous, debug); }
	}
}

#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, false)
#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false, true)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog(__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true, true)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#endif
