#include "main.h"
#include <cage-core/concurrent.h>

namespace
{
	void throwingFunction()
	{
		CAGE_THROW_ERROR(exception, "throwing function");
	}
}

void testExceptions()
{
	CAGE_TESTCASE("exceptions");
	detail::overrideBreakpoint overrideBreakpoint;

	{
		uint32 catches = 0;
		CAGE_TESTCASE("exception re-throw and catch");
		try
		{
			try
			{
				CAGE_THROW_ERROR(codeException, "intentional", 42);
			}
			catch (const exception &e)
			{
				CAGE_TEST(detail::strcmp(e.message, "intentional") == 0);
				catches++;
				throw;
			}
		}
		catch (const codeException &e)
		{
			CAGE_TEST(detail::strcmp(e.message, "intentional") == 0);
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
		detail::setGlobalBreakpointOverride(false);
		try
		{
			CAGE_TESTCASE("exception transfer between threads");
			holder<threadClass> thr = newThread(delegate<void()>().bind<&throwingFunction>(), "throwing thread");
			CAGE_TEST_THROWN(thr->wait());
		}
		catch (...)
		{
			// do nothing
		}
		detail::setGlobalBreakpointOverride(true);
	}
}