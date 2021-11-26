#include <cage-core/networkTcp.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>
#include <cage-core/config.h>

#include "database.h"

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
}

void notifierInitialize()
{
	notifierInstance = systemMemory().createHolder<Notifier>(configNotifierPort);
}

void notifierAcceptConnections()
{
	if (notifierInstance)
		notifierInstance->accept();
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
