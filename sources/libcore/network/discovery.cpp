#include "net.h"
#include <cage-core/guid.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/flatSet.h>

#include <vector>
#include <set>
#include <map>

namespace cage
{
	using namespace privat;

	namespace
	{
		constexpr uint32 IdSize = 64;

		// AF_INET = ipv4 only, AF_INET6 = ipv6 only, AF_UNSPEC = both
		constexpr int ProtocolFamily = AF_UNSPEC;

		struct PeerStruct : public DiscoveryPeer
		{
			uint32 ttl = 0;
		};

		struct SockAddrs
		{
			FlatSet<Addr> addresses;
			Sock s;

			SockAddrs(int family, int type, int protocol) : s(family, type, protocol)
			{}
		};

		class DiscoveryClientImpl : public DiscoveryClient
		{
		public:
			std::map<Guid<IdSize>, PeerStruct> peers;
			std::vector<SockAddrs> sockets;
			uint32 gameId = 0;
			uint16 sendPort = 0;

			DiscoveryClientImpl(uint16 sendPort, uint32 gameId) : gameId(gameId), sendPort(sendPort)
			{
				AddrList l(nullptr, 0, ProtocolFamily, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (l.valid())
				{
					int family = -1, type = -1, protocol = -1;
					Addr address;
					l.getAll(address, family, type, protocol);
					SockAddrs s(family, type, protocol);
					s.s.setBlocking(false);
					s.s.setBroadcast(true);
					s.s.setReuseaddr(true);
					s.s.bind(address);
					sockets.push_back(std::move(s));
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
				AddrList l(address.c_str(), port, ProtocolFamily, SOCK_DGRAM, IPPROTO_UDP, 0);
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
					PeerStruct &p = it->second;
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
				Addr a;
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
							s.s = Sock();
							break;
						}
						if (buffer.size() == 0)
							break;
						try
						{
							PeerStruct p;
							Guid<IdSize> id;
							uint32 gid;
							Deserializer d(buffer);
							d >> gid >> id >> p.port;
							auto av = numeric_cast<uint32>(d.available());
							auto ap = d.read(av).data();
							p.message = string({ ap, ap + av });
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

		class DiscoveryServerImpl : public DiscoveryServer
		{
		public:
			Guid<IdSize> uniId;
			uint32 gameId;
			uint16 gamePort;
			std::vector<Sock> sockets;

			DiscoveryServerImpl(uint16 listenPort, uint16 gamePort, uint32 gameId) : uniId(true), gameId(gameId), gamePort(gamePort)
			{
				AddrList l(nullptr, listenPort, ProtocolFamily, SOCK_DGRAM, IPPROTO_UDP, AI_PASSIVE);
				while (l.valid())
				{
					int family = -1, type = -1, protocol = -1;
					Addr address;
					l.getAll(address, family, type, protocol);
					Sock s(family, type, protocol);
					s.setBlocking(false);
					s.setBroadcast(true);
					s.setReuseaddr(true);
					s.bind(address);
					sockets.push_back(std::move(s));
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
				Addr a;
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
							s = Sock();
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
								ser.write({ message.c_str(), message.c_str() + message.length() });
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
		DiscoveryClientImpl *impl = (DiscoveryClientImpl *)this;
		impl->update();
	}

	void DiscoveryClient::addServer(const string &address, uint16 port)
	{
		DiscoveryClientImpl *impl = (DiscoveryClientImpl *)this;
		impl->addServer(address, port);
	}

	uint32 DiscoveryClient::peersCount() const
	{
		const DiscoveryClientImpl *impl = (const DiscoveryClientImpl *)this;
		return numeric_cast<uint32>(impl->peers.size());
	}

	DiscoveryPeer DiscoveryClient::peerData(uint32 index) const
	{
		DiscoveryClientImpl *impl = (DiscoveryClientImpl*)this;
		CAGE_ASSERT(index < impl->peers.size());
		auto it = impl->peers.begin();
		std::advance(it, index);
		return it->second;
	}

	Holder<PointerRange<DiscoveryPeer>> DiscoveryClient::peers() const
	{
		const DiscoveryClientImpl *impl = (const DiscoveryClientImpl *)this;
		PointerRangeHolder<DiscoveryPeer> h;
		h.reserve(impl->peers.size());
		for (const auto &it : impl->peers)
			h.push_back(it.second);
		return h;
	}

	Holder<DiscoveryClient> newDiscoveryClient(uint16 sendPort, uint32 gameId)
	{
		return systemMemory().createImpl<DiscoveryClient, DiscoveryClientImpl>(sendPort, gameId);
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
