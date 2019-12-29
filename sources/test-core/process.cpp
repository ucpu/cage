#include "main.h"
#include <cage-core/process.h>

void testProgram()
{
	CAGE_TESTCASE("process");
	string cmd;
#ifdef CAGE_SYSTEM_WINDOWS
	// on windows, echo is built-in process of cmd
	cmd = "cmd /C echo hi there";
#else
	cmd = "echo hi there";
#endif // CAGE_SYSTEM_WINDOWS
	Holder<Process> prg = newProcess(cmd);
	CAGE_TEST(prg->readLine() == "hi there");
	CAGE_TEST(prg->wait() == 0);
}
