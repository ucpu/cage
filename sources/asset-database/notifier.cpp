#include <list>

#include <cage-core/core.h>
#include <cage-core/network.h>
#include <cage-core/concurrent.h>

using namespace cage;

#include "notifier.h"

namespace
{
	class notifierClass
	{
	public:
		notifierClass(const uint16 port)
		{
			server = newTcpServer(port);
			mut = newSyncMutex();
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
			std::list<Holder<TcpConnection>>::iterator it = connections.begin();
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

	Holder<notifierClass> notifierInstance;
}

void notifierInitialize(const uint16 port)
{
	notifierInstance = detail::systemArena().createHolder<notifierClass>(port);
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
