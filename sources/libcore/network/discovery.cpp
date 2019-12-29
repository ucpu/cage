#include <vector>
#include <set>
#include <map>

#include "net.h"
#include <cage-core/guid.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/pointerRangeHolder.h>

namespace cage
{
	using namespace privat;

	DiscoveryPeer::DiscoveryPeer() : port(0)
	{}

	namespace
	{
		static const uint32 idSize = 64;

		// AF_INET = ipv4 only, AF_INET6 = ipv6 only, AF_UNSPEC = both
		static const int protocolFamily = AF_UNSPEC;

		struct peerStruct : public DiscoveryPeer
		{
			uint32 ttl;
		};

		struct sockStruct
		{
			std::set<addr> addresses;
			sock s;

			sockStruct(int family, int type, int protocol) : s(family, type, protocol)
			{}
		};

		class discoveryClientImpl : public DiscoveryClient
		{
		public:
			std::map<Guid<idSize>, peerStruct> peers;
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
				try
				{
					addServer("255.255.255.255", sendPort); // ipv4 broadcast
				}
				catch (const cage::Exception &)
				{
					// nothing
				}
			}

			void addServer(const string &address, uint16 port)
			{
				addrList l(address.c_str(), port, protocolFamily, SOCK_DGRAM, IPPROTO_UDP, 0);
				while (l.valid())
				{
					for (auto &it : sockets)
						if (it.s.getFamily() == l.family())
							it.addresses.insert(l.address());
					l.next();
				}
			}

			void ttlPeers()
			{
				auto it = peers.begin();
				while (it != peers.end())
				{
					peerStruct &p = it->second;
					if (p.ttl-- == 0)
						it = peers.erase(it);
					else
						it++;
				}
			}

			void cleanSockets()
			{
				auto it = sockets.begin();
				while (it != sockets.end())
				{
					if (!it->s.isValid())
						it = sockets.erase(it);
					else
						it++;
				}
			}

			void receive()
			{
				addr a;
				MemoryBuffer buffer;
				for (auto &s : sockets)
				{
					while (true)
					{
						buffer.resize(1024);
						try
						{
							buffer.resize(s.s.recvFrom(buffer.data(), buffer.size(), a, 0));
						}
						catch (const cage::Exception &)
						{
							s.s = sock();
							break;
						}
						if (buffer.size() == 0)
							break;
						try
						{
							peerStruct p;
							Guid<idSize> id;
							uint32 gid;
							Deserializer d(buffer);
							d >> gid >> id >> p.port;
							auto av = numeric_cast<uint32>(d.available());
							p.message = string((char*)d.advance(av), av);
							auto pit = peers.find(id);
							if (pit == peers.end())
							{
								uint16 dummyPort;
								a.translate(p.address, dummyPort);
								p.ttl = 5;
								peers[id] = p;
							}
							else
							{
								pit->second.ttl = 5;
								pit->second.message = p.message;
							}
						}
						catch (const cage::Exception &)
						{
							// ignore invalid message
						}
					}
				}
				cleanSockets();
			}

			void send()
			{
				for (auto &s : sockets)
				{
					for (auto &a : s.addresses)
					{
						try
						{
							s.s.sendTo(&gameId, 4, a);
						}
						catch (const cage::Exception &)
						{
							// nothing
						}
					}
				}
			}

			void update()
			{
				detail::OverrideBreakpoint OverrideBreakpoint;
				ttlPeers();
				receive();
				send();
			}
		};

		class discoveryServerImpl : public DiscoveryServer
		{
		public:
			Guid<idSize> uniId;
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

			void cleanSockets()
			{
				auto it = sockets.begin();
				while (it != sockets.end())
				{
					if (!it->isValid())
						it = sockets.erase(it);
					else
						it++;
				}
			}

			void update()
			{
				detail::OverrideBreakpoint OverrideBreakpoint;
				MemoryBuffer buffer;
				addr a;
				for (auto &s : sockets)
				{
					while (true)
					{
						buffer.resize(256);
						try
						{
							buffer.resize(s.recvFrom(buffer.data(), buffer.size(), a, 0));
						}
						catch (const cage::Exception &)
						{
							s = sock();
							break;
						}
						if (buffer.size() == 0)
							break;
						try
						{
							{ // parse
								Deserializer d(buffer);
								uint32 gid;
								d >> gid;
								if (gid != gameId)
									continue; // message not for us
								if (s.available() != 0)
									continue; // message too long
							}
							{ // respond
								buffer.clear();
								Serializer ser(buffer);
								ser << uniId << gameId << gamePort;
								ser.write(message.c_str(), message.length());
								s.sendTo(buffer.data(), buffer.size(), a);
							}
						}
						catch (const cage::Exception &)
						{
							continue; // ignore invalid messages
						}
					}
				}
				cleanSockets();
			}
		};
	}

	void DiscoveryClient::update()
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		impl->update();
	}

	void DiscoveryClient::addServer(const string &address, uint16 port)
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		impl->addServer(address, port);
	}

	uint32 DiscoveryClient::peersCount() const
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		return numeric_cast<uint32>(impl->peers.size());
	}

	DiscoveryPeer DiscoveryClient::peerData(uint32 index) const
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		CAGE_ASSERT(index < impl->peers.size());
		auto it = impl->peers.begin();
		std::advance(it, index);
		return it->second;
	}

	Holder<PointerRange<DiscoveryPeer>> DiscoveryClient::peers() const
	{
		discoveryClientImpl *impl = (discoveryClientImpl*)this;
		PointerRangeHolder<DiscoveryPeer> h;
		h.reserve(impl->peers.size());
		for (const auto &it : impl->peers)
			h.push_back(it.second);
		return h;
	}

	void DiscoveryServer::update()
	{
		discoveryServerImpl *impl = (discoveryServerImpl*)this;
		impl->update();
	}

	Holder<DiscoveryClient> newDiscoveryClient(uint16 sendPort, uint32 gameId)
	{
		return detail::systemArena().createImpl<DiscoveryClient, discoveryClientImpl>(sendPort, gameId);
	}

	Holder<DiscoveryServer> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId)
	{
		return detail::systemArena().createImpl<DiscoveryServer, discoveryServerImpl>(listenPort, gamePort, gameId);
	}
}
