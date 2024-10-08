#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/logger.h>
#include <cage-core/process.h>

using namespace cage;

int main(int argc, const char *args[])
{
	initializeConsoleLogger();
	try
	{
		while (true)
		{
			ProcessCreateConfig cfg("cage-asset-database");
			cfg.discardStdErr = cfg.discardStdIn = cfg.discardStdOut = true;
			Holder<Process> proc = newProcess(cfg);
			const auto res = proc->wait();
			if (res != 0)
				CAGE_THROW_ERROR(Exception, "asset database failed");

			pathRemove("process-log");
			pathRemove("../assets.zip");
			pathRemove("../data/database");

			// give some time for the filesystem
			threadSleep(1000 * 1000);
		}
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
