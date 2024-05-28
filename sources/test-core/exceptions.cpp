#include <cstring> // strcmp

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/string.h>

namespace
{
	void throwingFunction()
	{
		CAGE_THROW_ERROR(Exception, "throwing function");
	}

	bool assertFailureFunction()
	{
		CAGE_ASSERT(false); // this line number
		return true;
	}

	bool assertPassFunction()
	{
		CAGE_ASSERT(true);
		return true;
	}
}

void testExceptions()
{
	CAGE_TESTCASE("exceptions");
	detail::OverrideBreakpoint OverrideBreakpoint;

	{
		CAGE_TESTCASE("exception re-throw and catch");
		uint32 catches = 0;
		try
		{
			try
			{
				CAGE_THROW_ERROR(SystemError, "intentional", 42);
			}
			catch (const Exception &e)
			{
				CAGE_TEST(std::strcmp(e.message, "intentional") == 0);
				catches++;
				throw;
			}
		}
		catch (const SystemError &e)
		{
			CAGE_TEST(std::strcmp(e.message, "intentional") == 0);
			CAGE_TEST(e.code == 42);
			catches++;
		}
		catch (...)
		{
			CAGE_TEST(false);
		}
		CAGE_TEST(catches == 2);
	}

	{
		CAGE_TESTCASE("exception file");
		try
		{
			CAGE_THROW_ERROR(Exception, "intentional"); // this line number
		}
		catch (const Exception &e)
		{
			CAGE_TEST(std::strcmp(e.location.file_name(), __FILE__) == 0);
			CAGE_TEST(e.location.line() == 67); // marked line number
			CAGE_TEST(isPattern(String(e.location.function_name()), "", "testExceptions", ""));
			CAGE_TEST(std::strcmp(e.message, "intentional") == 0);
			CAGE_TEST(e.severity == SeverityEnum::Error);
		}
	}

	{
		CAGE_TESTCASE("assert");
		CAGE_TEST(assertPassFunction());
		CAGE_TEST_ASSERTED(assertFailureFunction());

#ifdef CAGE_ASSERT_ENABLED
		{
			CAGE_TESTCASE("assert exception file");
			try
			{
				detail::OverrideAssert ovr;
				assertFailureFunction();
			}
			catch (const Exception &e)
			{
				CAGE_TEST(std::strcmp(e.location.file_name(), __FILE__) == 0);
				CAGE_TEST(e.location.line() == 18); // marked line number
				CAGE_TEST(isPattern(String(e.location.function_name()), "", "assertFailureFunction", ""));
				CAGE_TEST(e.severity == SeverityEnum::Critical);
			}
		}
#endif // CAGE_ASSERT_ENABLED
	}

	{
		CAGE_TESTCASE("exception transfer between threads");
		detail::globalBreakpointOverride(false);
		try
		{
			Holder<Thread> thr = newThread(Delegate<void()>().bind<&throwingFunction>(), "throwing thread");
			CAGE_TEST_THROWN(thr->wait());
		}
		catch (...)
		{
			// do nothing
		}
		detail::globalBreakpointOverride(true);
	}

	{
		CAGE_TESTCASE("scope guard");
		{
			CAGE_TESTCASE("regular flow");
			int cnt = 0;
			{
				ScopeGuard g([&] { cnt++; });
				CAGE_TEST(cnt == 0);
			}
			CAGE_TEST(cnt == 1);
		}
		{
			CAGE_TESTCASE("exceptional flow");
			int cnt = 0;
			try
			{
				ScopeGuard g([&] { cnt++; });
				CAGE_TEST(cnt == 0);
				throw "okay";
			}
			catch (...)
			{
				// nothing
			}
			CAGE_TEST(cnt == 1);
		}
	}
}
