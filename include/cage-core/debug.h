#ifndef guard_debug_h_564gv56esr4tfj
#define guard_debug_h_564gv56esr4tfj

#include "core.h"

namespace cage
{
	namespace detail
	{
		CAGE_CORE_API void terminate();
		CAGE_CORE_API void debugOutput(const string &msg);
		CAGE_CORE_API void debugBreakpoint();

		// makes all debugBreakpoint calls be ignored
		struct CAGE_CORE_API OverrideBreakpoint
		{
			explicit OverrideBreakpoint(bool enable = false);
			~OverrideBreakpoint();

		private:
			bool original;
		};

		// make assert failures throw a critical exception instead of terminating the application
		struct CAGE_CORE_API OverrideAssert
		{
			explicit OverrideAssert(bool deadly = false);
			~OverrideAssert();

		private:
			bool original;
		};

		// changes threshold for exception severity for logging
		struct CAGE_CORE_API OverrideException
		{
			explicit OverrideException(SeverityEnum severity = SeverityEnum::Critical);
			~OverrideException();

		private:
			SeverityEnum original;
		};

		CAGE_CORE_API void setGlobalBreakpointOverride(bool enable);
		CAGE_CORE_API void setGlobalAssertOverride(bool enable);
		CAGE_CORE_API void setGlobalExceptionOverride(SeverityEnum severity);
	}
}

#endif // guard_debug_h_564gv56esr4tfj