#include <cage-core/networkTcp.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include "database.h"

#include <vector>

extern ConfigString configPathInput;
extern ConfigSint32 configNotifierPort;
extern std::map<String, Holder<Asset>, StringComparatorFast> assets;

void checkAssets();
bool isNameIgnored(const String &name);

namespace
{
	class Notifier
	{
	public:
		Notifier(const uint16 port)
		{
			server = newTcpServer(port);
			mut = newMutex();
		}

		void accept()
		{
			ScopeLock lck(mut);
			Holder<TcpConnection> tmp = server->accept();
			if (tmp)
				connections.push_back(std::move(tmp));
		}

		void notify(const String &str)
		{
			detail::OverrideBreakpoint OverrideBreakpoint;
			ScopeLock lck(mut);
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

	private:
		Holder<TcpServer> server;
		std::vector<Holder<TcpConnection>> connections;
		Holder<Mutex> mut;
	};

	Holder<Notifier> notifierInstance;

	void notifierAcceptConnections()
	{
		if (notifierInstance)
			notifierInstance->accept();
	}
}

void notifierInitialize()
{
	notifierInstance = systemMemory().createHolder<Notifier>(configNotifierPort);
}

void notifierSendNotifications()
{
	for (auto &it : assets)
	{
		if (it.second->needNotify)
		{
			CAGE_ASSERT(!it.second->corrupted);
			if (notifierInstance)
				notifierInstance->notify(it.first);
			it.second->needNotify = false;
		}
	}
}

void listen()
{
	CAGE_LOG(SeverityEnum::Info, "database", "initializing listening for changes");
	notifierInitialize();
	Holder<FilesystemWatcher> changes = newFilesystemWatcher();
	changes->registerPath(configPathInput);
	uint32 cycles = 0;
	bool changed = false;
	while (true)
	{
		notifierAcceptConnections();
		{
			String path = changes->waitForChange(1000 * 200);
			if (!path.empty())
			{
				path = pathToRel(path, configPathInput);
				if (isNameIgnored(path))
					continue;
				cycles = 0;
				changed = true;
			}
		}
		if (!changed)
			continue;
		if (cycles >= 3)
		{
			checkAssets();
			changed = false;
			cycles = 0;
		}
		else
			cycles++;
	}
}
