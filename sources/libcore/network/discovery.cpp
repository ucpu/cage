#include <vector>
#include <set>
#include <map>

#include "net.h"
#include <cage-core/utility/identifier.h>

namespace cage
{
	using namespace privat;

	namespace
	{
		static const uint32 idSize = 64;

		// AF_INET = ipv4 only, AF_INET6 = ipv6 only, AF_UNSPEC = both
		static const int protocolFamily = AF_INET;

		struct peerStruct
		{
			string message;
			string address;
			uint16 port;
			uint32 ttl;
		};

		struct sockStruct
		{
			std::set<addr> addresses;
			sock s;
			int family;

			sockStruct(int family, int type, int protocol) : s(family, type, protocol), family(family) {}
		};

		class discoveryClientImpl : public discoveryClientClass
		{
		public:
			std::map<identifier<idSize>, peerStruct> peers;
			std::vector<sockStruct> sockets;
			uint32 gameId;
			uint16 sendPort;

			discoveryClientImpl(uint16 sendPort, uint32 gameId) : gameId(gameId), sendPort(sendPort)
			{
				addrList l(nullptr, 0, protocolFamily, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (l.valid())
				{
					int family = -1, type = -1, protocol = -1;
					addr address;
					l.getAll(address, family, type, protocol);
					sockStruct s(family, type, protocol);
					s.s.setBlocking(false);
					s.s.setBroadcast(true);
					s.s.setReuseaddr(true);
					s.s.bind(address);
					sockets.push_back(templates::move(s));
					l.next();
				}
				addServer("255.255.255.255", sendPort); // ipv4 broadcast
			}
		};

		class discoveryServerImpl : public discoveryServerClass
		{
		public:
			identifier<idSize> uniId;
			uint32 gameId;
			uint16 gamePort;
			std::vector<sock> sockets;

			discoveryServerImpl(uint16 listenPort, uint16 gamePort, uint32 gameId) : uniId(true), gameId(gameId), gamePort(gamePort)
			{
				addrList l(nullptr, listenPort, protocolFamily, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (l.valid())
				{
					int family = -1, type = -1, protocol = -1;
					addr address;
					l.getAll(address, family, type, protocol);
					sock s(family, type, protocol);
					s.setBlocking(false);
					s.setBroadcast(true);
					s.setReuseaddr(true);
					s.bind(address);
					sockets.push_back(templates::move(s));
					l.next();
				}
			}
		};
	}

	void discoveryClientClass::update()
	{
		detail::overrideBreakpoint overrideBreakpoint;

		discoveryClientImpl *impl = (discoveryClientImpl*)this;

		{ // ttl
			std::map<identifier<idSize>, peerStruct>::iterator it = impl->peers.begin();
			while (it != impl->peers.end())
			{
				peerStruct &p = it->second;
				if (p.ttl-- == 0)
					it = impl->peers.erase(it);
				else
					it++;
			}
		}

		// receive
		{
			char buffer[256];
			addr a;
			peerStruct p;
			uint16 prt;
			std::vector<sockStruct>::iterator it = impl->sockets.begin();
			while (it != impl->sockets.end())
			{
				sockStruct &s = *it;
				try
				{
					while (true)
					{
						uintPtr len = s.s.recvFrom(buffer, 256, a);
						if (len == 0)
							break;
						if (len <= idSize + 6 || len >= 256)
							continue; // ignore, message has wrong length
						if (*(uint32*)(buffer + idSize) != impl->gameId)
							continue; // ignore, wrong game id
						identifier<idSize> id = *(identifier<idSize>*)buffer;
						std::map<identifier<idSize>, peerStruct>::iterator pit = impl->peers.find(id);
						if (pit == impl->peers.end())
						{
							a.translate(p.address, prt);
							p.port = *(uint16*)(buffer + idSize + 4);
							p.message = string(buffer + idSize + 6, numeric_cast<uint32>(len) - idSize - 6);
							p.ttl = 5;
							impl->peers[id] = p;
						}
						else
							pit->second.ttl = 5;
					}
					it++;
				}
				catch (const cage::exception &)
				{
					it = impl->sockets.erase(it);
				}
			}
		}

		{ // send
			std::vector<sockStruct>::iterator it = impl->sockets.begin();
			while (it != impl->sockets.end())
			{
				sockStruct &s = *it;
				try
				{
					std::set<addr>::iterator ia = s.addresses.begin();
					while (ia != s.addresses.end())
					{
						try
						{
							s.s.sendTo(&impl->gameId, 4, *ia);
							ia++;
						}
						catch (...)
						{
							ia = s.addresses.erase(ia);
						}
					}
					it++;
				}
				catch (const cage::exception &)
				{
					it = impl->sockets.erase(it);
				}
			}
		}
	}

	void discoveryClientClass::addServer(const string &address, uint16 port)
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		addrList l(address.c_str(), port, protocolFamily, SOCK_DGRAM, IPPROTO_UDP, 0);
		while (l.valid())
		{
			for (auto &it : impl->sockets)
				if (it.family == l.family())
					it.addresses.insert(l.address());
			l.next();
		}
	}

	uint32 discoveryClientClass::peersCount() const
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		return numeric_cast<uint32>(impl->peers.size());
	}

	void discoveryClientClass::peerData(uint32 index, string &message, string &address, uint16 &port) const
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		std::map<identifier<idSize>, peerStruct>::iterator it = impl->peers.begin();
		std::advance(it, index);
		const peerStruct &p = it->second;
		message = p.message;
		address = p.address;
		port = p.port;
	}

	void discoveryServerClass::update()
	{
		detail::overrideBreakpoint overrideBreakpoint;

		discoveryServerImpl *impl = (discoveryServerImpl*)this;
		char buffer[256];
		addr a;
		std::vector<sock>::iterator it = impl->sockets.begin();
		while (it != impl->sockets.end())
		{
			sock &s = *it;
			try
			{
				while (true)
				{
					uintPtr len = s.recvFrom(buffer, 256, a);
					if (len == 0)
						break;
					if (len != 4)
						continue; // ignore, message has wrong length
					detail::memcpy(buffer, impl->uniId.data, idSize);
					detail::memcpy(buffer + idSize, &impl->gameId, 4);
					detail::memcpy(buffer + idSize + 4, &impl->gamePort, 2);
					detail::memcpy(buffer + idSize + 6, impl->message.c_str(), impl->message.length());
					s.sendTo(buffer, idSize + 6 + impl->message.length(), a);
				}
				it++;
			}
			catch (const cage::exception &)
			{
				it = impl->sockets.erase(it);
			}
		}
	}

	holder<discoveryClientClass> newDiscoveryClient(uint16 sendPort, uint32 gameId)
	{
		return detail::systemArena().createImpl<discoveryClientClass, discoveryClientImpl>(sendPort, gameId);
	}

	holder<discoveryServerClass> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId)
	{
		return detail::systemArena().createImpl<discoveryServerClass, discoveryServerImpl>(listenPort, gamePort, gameId);
	}
}
