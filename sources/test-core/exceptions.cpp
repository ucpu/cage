#include "main.h"
#include <cage-core/concurrent.h>

#include <cstring> // strcmp

namespace
{
	void throwingFunction()
	{
		CAGE_THROW_ERROR(Exception, "throwing function");
	}
}

void testExceptions()
{
	CAGE_TESTCASE("exceptions");
	detail::OverrideBreakpoint OverrideBreakpoint;

	{
		uint32 catches = 0;
		CAGE_TESTCASE("exception re-throw and catch");
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
		detail::setGlobalBreakpointOverride(false);
		try
		{
			CAGE_TESTCASE("exception transfer between threads");
			Holder<Thread> thr = newThread(Delegate<void()>().bind<&throwingFunction>(), "throwing thread");
			CAGE_TEST_THROWN(thr->wait());
		}
		catch (...)
		{
			// do nothing
		}
		detail::setGlobalBreakpointOverride(true);
	}
}
