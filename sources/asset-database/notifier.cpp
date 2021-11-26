#include <cage-core/networkTcp.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include "database.h"

#include <list>

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
		std::list<Holder<TcpConnection>> connections;
		Holder<Mutex> mut;
	};

	Holder<Notifier> notifierInstance;
}

void notifierInitialize(const uint16 port)
{
	notifierInstance = systemMemory().createHolder<Notifier>(port);
}

void notifierAccept()
{
	if (notifierInstance)
		notifierInstance->accept();
}

void notifierNotify(const String &str)
{
	if (notifierInstance)
		notifierInstance->notify(str);
}
