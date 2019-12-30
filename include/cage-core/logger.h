#ifndef guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
#define guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_

namespace cage
{
	namespace detail
	{
		struct CAGE_API LoggerInfo
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
			SeverityEnum severity;
			bool continuous;
			bool debug;
			LoggerInfo();
		};
	}

	class CAGE_API Logger : private Immovable
	{
	public:
		Delegate<bool(const detail::LoggerInfo &)> filter;
		Delegate<void(const detail::LoggerInfo &, Delegate<void(const string &)>)> format;
		Delegate<void(const string &)> output;
	};

	CAGE_API Holder<Logger> newLogger();

	CAGE_API void logFormatConsole(const detail::LoggerInfo &info, Delegate<void(const string &)> output);
	CAGE_API void logFormatFileShort(const detail::LoggerInfo &info, Delegate<void(const string &)> output);
	CAGE_API void logFormatFileLong(const detail::LoggerInfo &info, Delegate<void(const string &)> output);

	CAGE_API void logOutputDebug(const string &message);
	CAGE_API void logOutputStdOut(const string &message);
	CAGE_API void logOutputStdErr(const string &message);

	class CAGE_API LoggerOutputFile : private Immovable
	{
	public:
		void output(const string &message);
	};

	CAGE_API Holder<LoggerOutputFile> newLoggerOutputFile(const string &path, bool append);

	namespace detail
	{
		CAGE_API Logger *getApplicationLog();
		CAGE_API string severityToString(const SeverityEnum severity);
	}
}

#endif // guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
