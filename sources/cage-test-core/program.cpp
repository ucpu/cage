#include "main.h"
#include <cage-core/utility/program.h>

void testProgram()
{
	CAGE_TESTCASE("program");
	string cmd;
#ifdef CAGE_SYSTEM_WINDOWS
	// on windows, echo is built-in program of cmd
	cmd = "cmd /C echo hi there";
#else
	cmd = "echo hi there";
#endif // CAGE_SYSTEM_WINDOWS
	holder<programClass> prg = newProgram(cmd);
	CAGE_TEST(prg->readLine() == "hi there");
	CAGE_TEST(prg->wait() == 0);
}
