#include <cage-core/core.h>
#include <cage-core/log.h>

using namespace cage;

#define CAGE_TESTCASE(NAME) CAGE_LOG(severityEnum::Info, "testcase", string().fill(cageTestCaseStruct::counter * 4, ' ') + NAME); cageTestCaseStruct CAGE_JOIN(cageTestCase_, __LINE__);
#define CAGE_TEST(COND,...) { if (!(COND)) CAGE_THROW_CRITICAL(exception, CAGE_STRINGIZE(COND)); }
#define CAGE_TEST_THROWN(COND,...) { bool ok = false; { CAGE_LOG(severityEnum::Info, "exception", "awaiting exception"); ::cage::detail::overrideBreakpoint overrideBreakpoint; try { COND; } catch (...) { ok = true; } } if (!ok) { CAGE_THROW_CRITICAL(exception, CAGE_STRINGIZE(COND)); } else { CAGE_LOG(severityEnum::Info, "exception", "the exception was expected"); } }
#ifdef CAGE_ASSERT_ENABLED
#define CAGE_TEST_ASSERTED(COND,...) { ::cage::detail::overrideAssert overrideAssert; CAGE_TEST_THROWN(COND); }
#else
#define CAGE_TEST_ASSERTED(COND,...) {}
#endif

struct cageTestCaseStruct
{
	static uint32 counter;
	cageTestCaseStruct()
	{
		counter++;
	}
	~cageTestCaseStruct()
	{
		counter--;
	}
};
