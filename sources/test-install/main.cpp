#include <cage-core/logger.h>

using namespace cage;

void testCageInstallConsistentPaths();

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	testCageInstallConsistentPaths();
	return 0;
}
