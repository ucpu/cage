#ifndef guard_debug_h_564gv56esr4tfj
#define guard_debug_h_564gv56esr4tfj

#include <cage-core/core.h>

namespace cage
{
	namespace detail
	{
		[[noreturn]] CAGE_CORE_API void terminate();
		CAGE_CORE_API bool isDebugging();
		CAGE_CORE_API void debugOutput(const String &msg);
		CAGE_CORE_API void debugBreakpoint();

		// makes all debugBreakpoint calls (in this thread) be ignored
		struct CAGE_CORE_API OverrideBreakpoint : private Immovable
		{
			[[nodiscard]] explicit OverrideBreakpoint(bool enable = false);
			~OverrideBreakpoint();

		private:
			bool original;
		};

		// make assert failures (in this thread) throw a critical exception instead of terminating the application
		struct CAGE_CORE_API OverrideAssert : private Immovable
		{
			[[nodiscard]] explicit OverrideAssert(bool deadly = false);
			~OverrideAssert();

		private:
			bool original;
		};

		// changes threshold for exception severity for logging (in this thread)
		struct CAGE_CORE_API OverrideException : private Immovable
		{
			[[nodiscard]] explicit OverrideException(SeverityEnum severity = SeverityEnum::Critical);
			~OverrideException();

		private:
			SeverityEnum original;
		};

		CAGE_CORE_API void globalBreakpointOverride(bool enable);
		CAGE_CORE_API void globalAssertOverride(bool enable);
		CAGE_CORE_API void globalExceptionOverride(SeverityEnum severity);
	}
}

#endif // guard_debug_h_564gv56esr4tfj
