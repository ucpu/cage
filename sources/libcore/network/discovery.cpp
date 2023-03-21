#include <cage-core/networkDiscovery.h>
#include <cage-core/guid.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/flatSet.h>
#include <cage-core/hashString.h>

#include "net.h"

#include <vector>
#include <algorithm>

namespace cage
{
	using namespace privat;

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		constexpr const char *EmptyAddress = "";
#else
		constexpr const char *EmptyAddress = nullptr;
#endif // CAGE_SYSTEM_WINDOWS
		constexpr uint32 ProtocolId = HashString("cage network discovery");
		constexpr uint32 IdSize = 64;

		struct Peer : public DiscoveryPeer
		{
			Guid<IdSize> guid;
			uint32 ttl = 0;

			bool operator < (const Peer &p) const
			{
				return guid < p.guid;
			}
		};

		class DiscoveryClientImpl : public DiscoveryClient
		{
		public:
			const uint32 gameId = 0;
			std::vector<Sock> sockets;
			FlatSet<Addr> servers;
			FlatSet<Peer> peers;

			DiscoveryClientImpl(uint16 listenPort, uint32 gameId) : gameId(gameId)
			{
				AddrList lst(EmptyAddress, 0, AF_INET, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (lst.valid())
				{
					Sock s(lst.family(), lst.type(), lst.protocol());
					s.setBlocking(false);
					s.setBroadcast(true);
					s.setReuseaddr(true);
					s.bind(lst.address());
					sockets.push_back(std::move(s));
					lst.next();
				}

				addServer("255.255.255.255", listenPort);
			}

			void addServer(const String &address, uint16 listenPort)
			{
				detail::OverrideBreakpoint OverrideBreakpoint;
				AddrList lst(address, listenPort, AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0);
				while (lst.valid())
				{
					servers.insert(lst.address());
					lst.next();
				}
			}

			void update()
			{
				std::erase_if((std::vector<Peer>&)peers, [](Peer &p) -> bool {
					CAGE_ASSERT(p.ttl > 0);
					return --p.ttl == 0;
				});
				detail::OverrideBreakpoint OverrideBreakpoint;
				MemoryBuffer buffer;
				for (Sock &s : sockets)
				{
					try
					{
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
							uint16 dummy;
							addr.translate(p.address, dummy, false);
							addr.translate(p.domain, dummy, true);
							p.ttl = 20;
							peers.erase(p);
							peers.insert(p);
						}
						{
							buffer.resize(0);
							Serializer ser(buffer);
							ser << ProtocolId << gameId;
							for (const Addr &a : servers)
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

		class DiscoveryServerImpl : public DiscoveryServer
		{
		public:
			const Guid<IdSize> guid = Guid<IdSize>(true);
			const uint32 gameId = 0;
			const uint16 gamePort = 0;
			std::vector<Sock> sockets;

			DiscoveryServerImpl(uint16 listenPort, uint16 gamePort, uint32 gameId) : gameId(gameId), gamePort(gamePort)
			{
				AddrList lst(EmptyAddress, listenPort, AF_INET, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (lst.valid())
				{
					Sock s(lst.family(), lst.type(), lst.protocol());
					s.setBlocking(false);
					s.setBroadcast(true);
					s.setReuseaddr(true);
					s.bind(lst.address());
					sockets.push_back(std::move(s));
					lst.next();
				}
			}

			void update()
			{
				detail::OverrideBreakpoint OverrideBreakpoint;
				MemoryBuffer buffer;
				for (Sock &s : sockets)
				{
					try
					{
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
							s.sendTo(buffer.data(), buffer.size(), addr);
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
