#include <vector>

#include <cage-core/assetsDatabase.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/networkTcp.h>

using namespace cage;

extern ConfigSint32 configNotifierPort;
extern Holder<AssetsDatabase> database;

namespace
{
	Holder<TcpServer> server;
	std::vector<Holder<TcpConnection>> connections;

	void accept()
	{
		Holder<TcpConnection> tmp = server->accept();
		if (tmp)
			connections.push_back(std::move(tmp));
	}

	void notify(const String &str)
	{
		detail::OverrideBreakpoint OverrideBreakpoint;
		auto it = connections.begin();
		while (it != connections.end())
		{
			try
			{
				(*it)->writeLine(str);
				it++;
			}
			catch (const Exception &)
			{
				it = connections.erase(it);
			}
		}
	}

	void notifyAll()
	{
		if (database->status().modifiedAssets == 0)
			return;

		for (const auto &it : database->modifiedAssets())
		{
			if (database->asset(it)->corrupted)
				continue;
			notify(it);
		}

		database->markAllModified(false);
	}
}

void listen()
{
	server = newTcpServer(configNotifierPort);

	Holder<FilesystemWatcher> changes = newFilesystemWatcher();
	changes->registerPath(database->config().inputPath);

	uint32 cycles = 0;
	bool changed = false;
	while (true)
	{
		accept();
		{
			String path = changes->waitForChange(1000 * 200);
			if (!path.empty())
			{
				path = pathToRel(path, database->config().inputPath);
				if (database->isIgnored(path))
					continue;
				cycles = 0;
				changed = true;
			}
		}
		if (!changed)
			continue;
		if (cycles >= 3)
		{
			database->convertAssets();
			notifyAll();
			changed = false;
			cycles = 0;
		}
		else
			cycles++;
	}
}
