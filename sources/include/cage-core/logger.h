#ifndef guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
#define guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_

#include <cage-core/core.h>

namespace cage
{
	class File;

	namespace detail
	{
		struct CAGE_CORE_API LoggerInfo
		{
			String currentThreadName;
			std::source_location location;
			PointerRange<const char> message;
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
		Delegate<void(const detail::LoggerInfo &, Delegate<void(PointerRange<const char>)>)> format;
		Delegate<void(PointerRange<const char>)> output;
	};

	CAGE_CORE_API Holder<Logger> newLogger();

	CAGE_CORE_API void logFormatConsole(const detail::LoggerInfo &info, Delegate<void(PointerRange<const char>)> output);
	CAGE_CORE_API void logFormatFile(const detail::LoggerInfo &info, Delegate<void(PointerRange<const char>)> output);

	CAGE_CORE_API void logOutputDebug(PointerRange<const char> message);
	CAGE_CORE_API void logOutputStdOut(PointerRange<const char> message);
	CAGE_CORE_API void logOutputStdErr(PointerRange<const char> message);

	class CAGE_CORE_API LoggerOutputFile : private Immovable
	{
	public:
		void output(PointerRange<const char> message) const;
	};

	CAGE_CORE_API Holder<LoggerOutputFile> newLoggerOutputFile(const String &path, bool append, bool realFilesystemOnly = true);
	CAGE_CORE_API Holder<LoggerOutputFile> newLoggerOutputFile(Holder<File> file);

	CAGE_CORE_API StringPointer severityToString(const SeverityEnum severity);

	CAGE_CORE_API Logger *initializeConsoleLogger();

	namespace detail
	{
		CAGE_CORE_API Logger *globalLogger();
	}
}

#endif // guard_logger_h_0d9702a6_c7ca_4260_baff_7fc1b3c1dec5_
