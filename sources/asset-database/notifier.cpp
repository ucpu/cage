#include <cage-core/core.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

using namespace cage;

#include "notifier.h"

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
			ScopeLock<Mutex> lck(mut);
			Holder<TcpConnection> tmp = server->accept();
			if (tmp)
				connections.push_back(templates::move(tmp));
		}

		void notify(const string &str)
		{
			detail::OverrideBreakpoint OverrideBreakpoint;
			ScopeLock<Mutex> lck(mut);
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
	notifierInstance = detail::systemArena().createHolder<Notifier>(port);
}

void notifierAccept()
{
	if (notifierInstance)
		notifierInstance->accept();
}

void notifierNotify(const string &str)
{
	if (notifierInstance)
		notifierInstance->notify(str);
}
