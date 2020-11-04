#ifndef guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
#define guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_

#include "core.h"

namespace cage
{
	namespace detail
	{
		struct CAGE_CORE_API LoggerInfo
		{
			string message;
			string currentThreadName;
			const char *component = "";
			const char *file = nullptr;
			const char *function = nullptr;
			uint64 time = 0;
			uint64 createThreadId = 0;
			uint64 currentThreadId = 0;
			uint32 line = 0;
			SeverityEnum severity = SeverityEnum::Critical;
			bool continuous = false;
			bool debug = false;
		};
	}

	class CAGE_CORE_API Logger : private Immovable
	{
	public:
		Delegate<bool(const detail::LoggerInfo &)> filter;
		Delegate<void(const detail::LoggerInfo &, Delegate<void(const string &)>)> format;
		Delegate<void(const string &)> output;
	};

	CAGE_CORE_API Holder<Logger> newLogger();

	CAGE_CORE_API void logFormatConsole(const detail::LoggerInfo &info, Delegate<void(const string &)> output);
	CAGE_CORE_API void logFormatFileShort(const detail::LoggerInfo &info, Delegate<void(const string &)> output);
	CAGE_CORE_API void logFormatFileLong(const detail::LoggerInfo &info, Delegate<void(const string &)> output);

	CAGE_CORE_API void logOutputDebug(const string &message);
	CAGE_CORE_API void logOutputStdOut(const string &message);
	CAGE_CORE_API void logOutputStdErr(const string &message);

	class CAGE_CORE_API LoggerOutputFile : private Immovable
	{
	public:
		void output(const string &message);
	};

	CAGE_CORE_API Holder<LoggerOutputFile> newLoggerOutputFile(const string &path, bool append, bool realFilesystemOnly = true);

	namespace detail
	{
		CAGE_CORE_API Logger *getApplicationLog();
		CAGE_CORE_API string severityToString(const SeverityEnum severity);
	}
}

#endif // guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
