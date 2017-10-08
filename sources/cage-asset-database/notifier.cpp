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
			mut = newMutex();
		}

		void accept()
		{
			scopeLock<mutexClass> lck(mut);
			holder<tcpConnectionClass> tmp = server->accept();
			if (tmp)
				connections.push_back(tmp);
		}

		void notify(const string &str)
		{
			detail::overrideBreakpoint overrideBreakpoint;
			scopeLock<mutexClass> lck(mut);
			std::list<holder<tcpConnectionClass>>::iterator it = connections.begin();
			while (it != connections.end())
			{
				try
				{
					(*it)->writeLine(str);
					it++;
				}
				catch (const exception &)
				{
					it = connections.erase(it);
				}
			}
		}

	private:
		holder<tcpServerClass> server;
		std::list<holder<tcpConnectionClass>> connections;
		holder<mutexClass> mut;
	};

	holder<notifierClass> notifierInstance;
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