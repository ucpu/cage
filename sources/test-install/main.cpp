#include <cage-core/logger.h>

using namespace cage;

void testCageInstallConsistentPaths();

int main(int argc, const char *args[])
{
	Holder<Logger> log = newLogger();
	log->format.bind<logFormatConsole>();
	log->output.bind<logOutputStdOut>();
	testCageInstallConsistentPaths();
	return 0;
}
