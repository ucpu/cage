#include <cage-core/debug.h>

using namespace cage;

#define CAGE_TESTCASE(NAME) \
	{ \
		if (!std::is_constant_evaluated()) \
		{ \
			CAGE_LOG(SeverityEnum::Info, "testcase", Stringizer() + "TESTCASE: " + NAME); \
		} \
	}
#define CAGE_TEST(COND, ...) \
	{ \
		if (!(COND)) \
		{ \
			if (std::is_constant_evaluated()) \
			{ \
				throw; \
			} \
			else \
			{ \
				CAGE_LOG(SeverityEnum::Info, "test", #COND); \
				CAGE_THROW_CRITICAL(Exception, "test failed"); \
			} \
		} \
	}
#define CAGE_TEST_THROWN(COND, ...) \
	{ \
		bool ok = false; \
		{ \
			::cage::detail::OverrideBreakpoint OverrideBreakpoint; \
			try \
			{ \
				COND; \
			} \
			catch (...) \
			{ \
				ok = true; \
			} \
		} \
		if (!ok) \
		{ \
			CAGE_LOG(SeverityEnum::Error, "exception", "expected exception not caught"); \
			CAGE_THROW_CRITICAL(Exception, #COND); \
		} \
	}
#ifdef CAGE_ASSERT_ENABLED
	#define CAGE_TEST_ASSERTED(COND, ...) \
		{ \
			::cage::detail::OverrideAssert overrideAssert; \
			CAGE_TEST_THROWN(COND); \
		}
#else
	#define CAGE_TEST_ASSERTED(COND, ...) \
		{}
#endif
