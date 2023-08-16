#include <algorithm>
#include <vector>

#include "net.h"

#include <cage-core/flatSet.h>
#include <cage-core/guid.h>
#include <cage-core/hashString.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/networkDiscovery.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>

namespace cage
{
	using namespace privat;

	namespace
	{
		constexpr uint32 ProtocolId = HashString("cage network discovery");
		constexpr uint32 IdSize = 64;

		// ProtocolId is used for client->server requests
		// (ProtocolId + 1) is used for server->client responses
		// listenPort is used on server, listening to client broadcasts
		// (listenPort + 1) is used on clients, listening to server broadcasts

		struct Peer : public DiscoveryPeer
		{
			Guid<IdSize> guid;
			uint32 ttl = 0;

			bool operator<(const Peer &p) const { return guid < p.guid; }
		};

		class DiscoveryClientImpl : public DiscoveryClient
		{
		public:
			const uint32 gameId = 0;
			std::vector<Sock> sockets;
			FlatSet<Peer> peers;
			FlatSet<Addr> targets;

			DiscoveryClientImpl(uint16 listenPort, uint32 gameId) : gameId(gameId)
			{
				detail::OverrideBreakpoint ob;
				if (listenPort == m)
					listenPort--;
				const auto &add = [&](AddrList &&lst)
				{
					for (; lst.valid(); lst.next())
					{
						try
						{
							Sock s = lst.sock();
							s.setBlocking(false);
							s.setBroadcast(true);
							s.setReuseaddr(true);
							s.bind(lst.address());
							sockets.push_back(std::move(s));
						}
						catch (...)
						{
							// nothing
						}
					}
				};
				add(AddrList(EmptyAddress, 0, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE)); // searching server with direct addresses can use both ipv4 and ipv6
				add(AddrList(EmptyAddress, listenPort + 1, AF_INET, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE)); // broadcast can use ipv4 only
				addServer("255.255.255.255", listenPort);
			}

			void addServer(const String &address, uint16 listenPort)
			{
				detail::OverrideException overrideException;
				AddrList lst(address, listenPort, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, 0);
				while (lst.valid())
				{
					targets.insert(lst.address());
					lst.next();
				}
			}

			void update()
			{
				detail::OverrideException overrideException;
				std::erase_if((std::vector<Peer> &)peers,
					[](Peer &p) -> bool
					{
						CAGE_ASSERT(p.ttl > 0);
						return --p.ttl == 0;
					});
				MemoryBuffer buffer;
				buffer.reserve(2000);
				for (Sock &s : sockets)
				{
					try
					{
						// client reads all responses from servers
						while (true)
						{
							buffer.resize(2000);
							Addr addr;
							buffer.resize(s.recvFrom(buffer.data(), buffer.size(), addr));
							if (buffer.size() == 0)
								break; // no more messages
							if (buffer.size() < 14 + IdSize)
								continue; // message is not intended for us
							Deserializer des(buffer);
							{
								uint32 protocolId = 0, g = 0;
								des >> protocolId >> g;
								if (protocolId != ProtocolId + 1 || g != gameId)
									continue; // message is not intended for us
							}
							Peer p;
							des >> p.guid >> p.port >> p.message;
							p.address = addr.translate(false).first;
							p.ttl = 20;
							peers.erase(p);
							peers.insert(p);
						}

						// client sends requests to all potential servers, including broadcast
						{
							buffer.resize(0);
							Serializer ser(buffer);
							ser << ProtocolId << gameId;
							for (const Addr &a : targets)
							{
								if (s.getFamily() == a.getFamily())
									s.sendTo(buffer.data(), buffer.size(), a);
							}
						}
					}
					catch (...)
					{
						// ignore
					}
				}
			}
		};

		class DiscoveryServerImpl : public DiscoveryServer
		{
		public:
			const Guid<IdSize> guid = Guid<IdSize>(true);
			const uint32 gameId = 0;
			const uint16 gamePort = 0;
			std::vector<Sock> sockets;
			FlatSet<Addr> targets;

			DiscoveryServerImpl(uint16 listenPort, uint16 gamePort, uint32 gameId) : gameId(gameId), gamePort(gamePort)
			{
				detail::OverrideBreakpoint ob;
				if (listenPort == m)
					listenPort--;

				// searching server with direct addresses can use both ipv4 and ipv6
				for (AddrList lst(EmptyAddress, listenPort, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE); lst.valid(); lst.next())
				{
					try
					{
						Sock s = lst.sock();
						s.setBlocking(false);
						s.setBroadcast(true);
						s.setReuseaddr(true);
						s.bind(lst.address());
						sockets.push_back(std::move(s));
					}
					catch (...)
					{
						// nothing
					}
				}

				// broadcast can use ipv4 only
				for (AddrList lst("255.255.255.255", listenPort + 1, AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0); lst.valid(); lst.next())
					targets.insert(lst.address());
			}

			void update()
			{
				detail::OverrideException overrideException;
				MemoryBuffer buffer;
				buffer.reserve(2000);
				for (Sock &s : sockets)
				{
					try
					{
						// server reads all requests and sends responses
						while (true)
						{
							buffer.resize(100);
							Addr addr;
							buffer.resize(s.recvFrom(buffer.data(), buffer.size(), addr));
							if (buffer.size() == 0)
								break; // no more messages
							{
								if (buffer.size() < 8)
									continue; // message is not intended for us
								Deserializer des(buffer);
								uint32 protocolId = 0, g = 0;
								des >> protocolId >> g;
								if (protocolId != ProtocolId || g != gameId)
									continue; // message is not intended for us
							}
							buffer.resize(0);
							Serializer ser(buffer);
							ser << (ProtocolId + 1) << gameId << guid << gamePort << message;
							try
							{
								s.sendTo(buffer.data(), buffer.size(), addr);
							}
							catch (...)
							{
								// ignore
							}
						}

						// server announces its presence with broadcast
						if (s.getFamily() == AF_INET)
						{
							buffer.resize(0);
							Serializer ser(buffer);
							ser << (ProtocolId + 1) << gameId << guid << gamePort << message;
							for (const Addr &a : targets)
							{
								try
								{
									s.sendTo(buffer.data(), buffer.size(), a);
								}
								catch (...)
								{
									// ignore
								}
							}
						}
					}
					catch (...)
					{
						// ignore
					}
				}
			}
		};
	}

	void DiscoveryClient::update()
	{
		DiscoveryClientImpl *impl = (DiscoveryClientImpl *)this;
		impl->update();
	}

	void DiscoveryClient::addServer(const String &address, uint16 listenPort)
	{
		DiscoveryClientImpl *impl = (DiscoveryClientImpl *)this;
		impl->addServer(address, listenPort);
	}

	Holder<PointerRange<DiscoveryPeer>> DiscoveryClient::peers() const
	{
		const DiscoveryClientImpl *impl = (const DiscoveryClientImpl *)this;
		PointerRangeHolder<DiscoveryPeer> h;
		h.reserve(impl->peers.size());
		for (const auto &it : impl->peers)
			h.push_back(it);
		return h;
	}

	Holder<DiscoveryClient> newDiscoveryClient(uint16 listenPort, uint32 gameId)
	{
		return systemMemory().createImpl<DiscoveryClient, DiscoveryClientImpl>(listenPort, gameId);
	}

	void DiscoveryServer::update()
	{
		DiscoveryServerImpl *impl = (DiscoveryServerImpl *)this;
		impl->update();
	}

	Holder<DiscoveryServer> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId)
	{
		return systemMemory().createImpl<DiscoveryServer, DiscoveryServerImpl>(listenPort, gamePort, gameId);
	}
}
