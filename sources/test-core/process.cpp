#include "main.h"
#include <cage-core/process.h>

void testProcess()
{
	CAGE_TESTCASE("process");

#ifdef CAGE_SYSTEM_WINDOWS
	// on windows, echo is built-in command of cmd
	const string cmdEcho = "cmd /C echo hi there";
	const string cmdLs = "cmd /C dir /Q";
#else
	const string cmdEcho = "echo hi there";
	const string cmdLs = "ls -la";
#endif // CAGE_SYSTEM_WINDOWS

	{
		CAGE_TESTCASE("echo");
		Holder<Process> prg = newProcess(cmdEcho);
		string line = prg->readLine();
		CAGE_TEST(line == "hi there");
		CAGE_TEST(prg->wait() == 0);
	}

	{
		CAGE_TESTCASE("list directory");
		Holder<Process> prg = newProcess(cmdLs);
		uint32 lines = 0;
		try
		{
			detail::OverrideBreakpoint ob;
			while (true)
			{
				string line = prg->readLine();
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "dir list", line);
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
		CAGE_TESTCASE("discarded input and output");
		ProcessCreateConfig cfg(cmdLs);
		cfg.discardStdIn = cfg.discardStdOut = true;
		Holder<Process> prg = newProcess(cfg);
		uint32 lines = 0;
		try
		{
			detail::OverrideBreakpoint ob;
			while (true)
			{
				string line = prg->readLine();
				CAGE_LOG_CONTINUE(SeverityEnum::Info, "dir list", line);
				lines++;
			}
		}
		catch (const ProcessPipeEof &)
		{
		}
		CAGE_TEST(lines == 0);
		CAGE_TEST(prg->wait() == 0);
	}
}
