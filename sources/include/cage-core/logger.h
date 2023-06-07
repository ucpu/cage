#ifndef guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
#define guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_

#include "core.h"

namespace cage
{
	class File;

	namespace detail
	{
		struct CAGE_CORE_API LoggerInfo
		{
			std::source_location location;
			String message;
			String currentThreadName;
			StringPointer component = "";
			uint64 time = 0;
			uint64 createThreadId = 0;
			uint64 currentThreadId = 0;
			SeverityEnum severity = SeverityEnum::Critical;
			bool continuous = false;
			bool debug = false;
		};
	}

	class CAGE_CORE_API Logger : private Immovable
	{
	public:
		Delegate<bool(const detail::LoggerInfo &)> filter;
		Delegate<void(const detail::LoggerInfo &, Delegate<void(const String &)>)> format;
		Delegate<void(const String &)> output;
	};

	CAGE_CORE_API Holder<Logger> newLogger();

	CAGE_CORE_API void logFormatConsole(const detail::LoggerInfo &info, Delegate<void(const String &)> output);
	CAGE_CORE_API void logFormatFileShort(const detail::LoggerInfo &info, Delegate<void(const String &)> output);
	CAGE_CORE_API void logFormatFileLong(const detail::LoggerInfo &info, Delegate<void(const String &)> output);

	CAGE_CORE_API void logOutputDebug(const String &message);
	CAGE_CORE_API void logOutputStdOut(const String &message);
	CAGE_CORE_API void logOutputStdErr(const String &message);

	class CAGE_CORE_API LoggerOutputFile : private Immovable
	{
	public:
		void output(const String &message) const;
	};

	CAGE_CORE_API Holder<LoggerOutputFile> newLoggerOutputFile(const String &path, bool append, bool realFilesystemOnly = true);
	CAGE_CORE_API Holder<LoggerOutputFile> newLoggerOutputFile(Holder<File> file);

	namespace detail
	{
		CAGE_CORE_API Logger *globalLogger();
	}

	CAGE_CORE_API StringPointer severityToString(const SeverityEnum severity);
}

#endif // guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
