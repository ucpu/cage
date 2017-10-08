namespace cage
{
	namespace detail
	{
		struct CAGE_API loggerInfo
		{
			string component;
			string message;
			uint64 time;
			uint64 currentThread;
			uint64 createThread;
			severityEnum severity;
			bool continuous;
#ifdef CAGE_DEBUG
			const char *file;
			uint32 line;
			const char *function;
#endif
		};
	}

	template struct CAGE_API delegate<bool(const detail::loggerInfo &)>;
	template struct CAGE_API delegate<void(const detail::loggerInfo &, delegate<void(const string &)>)>;
	template struct CAGE_API delegate<void(const string &)>;

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
#ifdef CAGE_DEBUG
		CAGE_API uint64 makeLog(const char *file, uint32 line, const char *function, severityEnum severity, const string &component, const string &message, bool continuous) noexcept;
#else
		CAGE_API uint64 makeLog(severityEnum severity, const string &component, const string &message, bool continuous) noexcept;
#endif
		CAGE_API loggerClass *getCentralLog();
	}
}

#ifdef CAGE_DEBUG
#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog (__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog (__FILE__, __LINE__, __FUNCTION__, SEVERITY, COMPONENT, MESSAGE, true)
#else
#define CAGE_LOG(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog (SEVERITY, COMPONENT, MESSAGE, false)
#define CAGE_LOG_CONTINUE(SEVERITY, COMPONENT, MESSAGE) ::cage::detail::makeLog (SEVERITY, COMPONENT, MESSAGE, true)
#endif

#ifdef CAGE_DEBUG
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE) CAGE_LOG (SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE) CAGE_LOG_CONTINUE (SEVERITY, COMPONENT, MESSAGE)
#else
#define CAGE_LOG_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#define CAGE_LOG_CONTINUE_DEBUG(SEVERITY, COMPONENT, MESSAGE)
#endif
