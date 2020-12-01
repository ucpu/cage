#include "main.h"
#include <cage-core/process.h>

void testProcess()
{
	CAGE_TESTCASE("process");

	{
		CAGE_TESTCASE("echo");
#ifdef CAGE_SYSTEM_WINDOWS
		// on windows, echo is built-in command of cmd
		const string cmd = "cmd /C echo hi there";
#else
		const string cmd = "echo hi there";
#endif // CAGE_SYSTEM_WINDOWS
		Holder<Process> prg = newProcess(cmd);
		CAGE_TEST(prg->readLine() == "hi there");
		CAGE_TEST(prg->wait() == 0);
	}

	{
		CAGE_TESTCASE("echo 2");
#ifdef CAGE_SYSTEM_WINDOWS
		const string cmd = "cmd /C echo hi there";
#else
		const string cmd = "echo hi there";
#endif // CAGE_SYSTEM_WINDOWS
		Holder<Process> prg = newProcess(cmd);
		uint32 lines = 0;
		try
		{
			while (true)
			{
				detail::OverrideBreakpoint ob;
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "echo", prg->readLine());
				lines++;
			}
		}
		catch (const ProcessPipeEof &)
		{
			// nothing
		}
		CAGE_TEST(lines == 1);
		CAGE_TEST(prg->wait() == 0);
	}

	{
		CAGE_TESTCASE("list directory");
#ifdef CAGE_SYSTEM_WINDOWS
		const string cmd = "cmd /C dir /Q";
#else
		const string cmd = "ls -la";
#endif // CAGE_SYSTEM_WINDOWS
		Holder<Process> prg = newProcess(cmd);
		uint32 lines = 0;
		try
		{
			while (true)
			{
				detail::OverrideBreakpoint ob;
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "dir list", prg->readLine());
				lines++;
			}
		}
		catch (const ProcessPipeEof &)
		{
			// nothing
		}
		CAGE_TEST(lines > 2);
		CAGE_TEST(prg->wait() == 0);
	}

	{
		CAGE_TESTCASE("closed pipes");
#ifdef CAGE_SYSTEM_WINDOWS
		const string cmd = "cmd /C dir /Q";
#else
		const string cmd = "ls -la";
#endif // CAGE_SYSTEM_WINDOWS
		ProcessCreateConfig cfg(cmd);
		cfg.discardStdIn = cfg.discardStdOut = true;
		Holder<Process> prg = newProcess(cfg);
		uint32 lines = 0;
		try
		{
			while (true)
			{
				detail::OverrideBreakpoint ob;
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "dir list", prg->readLine());
				lines++;
			}
		}
		catch (const ProcessPipeEof &)
		{
			// nothing
		}
		CAGE_TEST(lines == 0);
		CAGE_TEST(prg->wait() == 0);
	}
}
