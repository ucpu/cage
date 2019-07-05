#include "main.h"
#include <cage-core/process.h>

void testProgram()
{
	CAGE_TESTCASE("processHandle");
	string cmd;
#ifdef CAGE_SYSTEM_WINDOWS
	// on windows, echo is built-in processHandle of cmd
	cmd = "cmd /C echo hi there";
#else
	cmd = "echo hi there";
#endif // CAGE_SYSTEM_WINDOWS
	holder<processHandle> prg = newProcess(cmd);
	CAGE_TEST(prg->readLine() == "hi there");
	CAGE_TEST(prg->wait() == 0);
}
