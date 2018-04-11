#include <list>

#include "net.h"
#include <cage-core/config.h>
#include <cage-core/math.h>

#include <enet/enet.h>

namespace cage
{
	namespace
	{
		configDouble simulatedPacketLoss("cage-core.udp.simulatedPacketLoss", 0);

		class enetInitializerClass
		{
		public:
			enetInitializerClass()
			{
				int err = enet_initialize();
				if (err != 0)
					CAGE_THROW_CRITICAL(codeException, "enet_initialize", err);
			}
			~enetInitializerClass()
			{
				enet_deinitialize();
			}
		} enetInitializerInstance;

		const int processHost(ENetHost *host, ENetPeer *&peer);

		class udpConnectionImpl : public udpConnectionClass
		{
		public:
			udpConnectionImpl(const string &address, uint16 port, uint32 channels, uint64 timeout) : client(nullptr), peer(nullptr)
			{
				ENetAddress addr;
				detail::memset(&addr, 0, sizeof(addr));
				if (enet_address_set_host(&addr, address.c_str()) != 0)
					CAGE_THROW_ERROR(exception, "host address resolution failed");
				addr.port = port;
				client = enet_host_create(nullptr, 1, channels, 0, 0);
				if (!client)
					CAGE_THROW_ERROR(exception, "udp client creation failed");
				peer = enet_host_connect(client, &addr, channels, 0);
				if (!peer)
					CAGE_THROW_ERROR(exception, "udp peer creation failed");
				peer->data = this;
				if (timeout > 0)
				{
					ENetEvent event;
					int ret = enet_host_service(client, &event, numeric_cast<uint32>(timeout / 1000));
					if (ret > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
						return;
					CAGE_THROW_ERROR(exception, "upd connection timed out");
				}
			}

			udpConnectionImpl(ENetPeer *peer) : client(nullptr), peer(peer)
			{
				peer->data = this;
			}

			~udpConnectionImpl()
			{
				while (!packets.empty())
				{
					enet_packet_destroy(packets.front().second);
					packets.pop_front();
				}
				if (peer)
				{
					peer->data = nullptr;
					enet_peer_reset(peer);
				}
				if (client)
					enet_host_destroy(client);
			}

			void check()
			{
				if (!peer)
				{
					detail::overrideBreakpoint ob;
					CAGE_THROW_ERROR(disconnectedException, "peer disconnected");
				}
				if (client)
				{
					ENetPeer *peer = nullptr;
					while (processHost(client, peer) != ENET_EVENT_TYPE_NONE);
				}
			}

			ENetHost *client;
			ENetPeer *peer;
			std::list<std::pair<uint32, ENetPacket*>> packets;
		};

		class udpServerImpl : public udpServerClass
		{
		public:
			udpServerImpl(uint16 port, uint32 channels, uint32 maxClients) : server(nullptr)
			{
				ENetAddress address;
				detail::memset(&address, 0, sizeof(address));
				enet_address_set_host(&address, "0.0.0.0");
				address.port = port;
				server = enet_host_create(&address, maxClients, channels, 0, 0);
				if (!server)
					CAGE_THROW_ERROR(exception, "udp server creation failed");
			}

			~udpServerImpl()
			{
				enet_host_destroy(server);
			}

			ENetHost *server;
		};

		const int processHost(ENetHost *host, ENetPeer *&peer)
		{
			ENetEvent event;
			if (enet_host_service(host, &event, 0) > 0)
			{
				peer = event.peer;
				switch (event.type)
				{
				case ENET_EVENT_TYPE_RECEIVE:
					if (event.packet->dataLength > 0)
						((udpConnectionImpl*)event.peer->data)->packets.push_back(std::pair<uint32, ENetPacket*>(event.channelID, event.packet));
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					((udpConnectionImpl*)event.peer->data)->peer = nullptr;
					enet_peer_reset(event.peer);
					break;
				default: // others do nothing
					break;
				}
				return event.type;
			}
			return ENET_EVENT_TYPE_NONE;
		}
	}

	uintPtr udpConnectionClass::available() const
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		impl->check();
		if (impl->packets.empty())
			return 0;
		return impl->packets.front().second->dataLength;
	}

	void udpConnectionClass::read(void *buffer, uintPtr size, uint32 &channel)
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		impl->check();
		CAGE_ASSERT_RUNTIME(!impl->packets.empty());
		CAGE_ASSERT_RUNTIME(size >= impl->packets.front().second->dataLength);
		detail::memcpy(buffer, impl->packets.front().second->data, impl->packets.front().second->dataLength);
		channel = impl->packets.front().first;
		enet_packet_destroy(impl->packets.front().second);
		impl->packets.pop_front();
	}

	void udpConnectionClass::read(void *buffer, uintPtr size)
	{
		uint32 channel;
		read(buffer, size, channel);
	}

	memoryBuffer udpConnectionClass::read(uint32 &channel)
	{
		memoryBuffer b(available());
		read(b.data(), b.size(), channel);
		return b;
	}

	memoryBuffer udpConnectionClass::read()
	{
		uint32 ch;
		return read(ch);
	}

	void udpConnectionClass::write(const void *buffer, uintPtr size, uint32 channel, bool reliable)
	{
		udpConnectionImpl *impl = (udpConnectionImpl*)this;
		if (!impl->peer)
			CAGE_THROW_ERROR(disconnectedException, "peer disconnected");
		ENetPacket *packet = enet_packet_create(buffer, size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
		if (!packet)
			CAGE_THROW_ERROR(exception, "packet creation failed");
		if (!reliable && cage::random() < (double)simulatedPacketLoss)
			return;
		if (enet_peer_send(impl->peer, numeric_cast<uint8>(channel), packet) != 0)
			CAGE_THROW_ERROR(exception, "packet send failed");
	}

	void udpConnectionClass::write(const memoryBuffer &buffer, uint32 channel, bool reliable)
	{
		write(buffer.data(), buffer.size(), channel, reliable);
	}

	holder<udpConnectionClass> udpServerClass::accept()
	{
		udpServerImpl *impl = (udpServerImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->server);
		while (true)
		{
			ENetPeer *peer = nullptr;
			switch (processHost(impl->server, peer))
			{
			case ENET_EVENT_TYPE_CONNECT:
				return detail::systemArena().createImpl<udpConnectionClass, udpConnectionImpl>(peer);
			case ENET_EVENT_TYPE_NONE:
				return holder<udpConnectionClass>();
			}
		}
	}

	holder<udpConnectionClass> newUdpConnection(const string &address, uint16 port, uint32 channels, uint64 timeout)
	{
		return detail::systemArena().createImpl<udpConnectionClass, udpConnectionImpl>(address, port, channels, timeout);
	}

	holder<udpServerClass> newUdpServer(uint16 port, uint32 channels, uint32 maxClients)
	{
		return detail::systemArena().createImpl<udpServerClass, udpServerImpl>(port, channels, maxClients);
	}
}
