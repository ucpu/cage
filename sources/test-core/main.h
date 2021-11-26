#include <cage-core/debug.h>
#include <cage-core/macros.h>
#include <cage-core/string.h>

using namespace cage;

#define CAGE_TESTCASE(NAME) { if (!std::is_constant_evaluated()) { CAGE_LOG(SeverityEnum::Info, "testcase", NAME); } }
#define CAGE_TEST(COND,...) { if (!(COND)) { if (std::is_constant_evaluated()) { throw; } else { CAGE_LOG(SeverityEnum::Info, "test", CAGE_STRINGIZE(COND)); CAGE_THROW_CRITICAL(Exception, "test failed"); } } }
#define CAGE_TEST_THROWN(COND,...) { bool ok = false; { CAGE_LOG(SeverityEnum::Info, "exception", "awaiting exception"); ::cage::detail::OverrideBreakpoint OverrideBreakpoint; try { COND; } catch (...) { ok = true; } } if (!ok) { CAGE_LOG(SeverityEnum::Info, "exception", "caught no exception"); CAGE_THROW_CRITICAL(Exception, CAGE_STRINGIZE(COND)); } else { CAGE_LOG(SeverityEnum::Info, "exception", "the exception was expected"); } }
#ifdef CAGE_ASSERT_ENABLED
#define CAGE_TEST_ASSERTED(COND,...) { ::cage::detail::OverrideAssert overrideAssert; CAGE_TEST_THROWN(COND); }
#else
#define CAGE_TEST_ASSERTED(COND,...) {}
#endif
