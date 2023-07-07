#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/endianness.h>
#include <cage-core/networkGinnel.h>

#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

#include "net.h"

namespace cage
{
	namespace
	{
		constexpr uint32 LanesCount = 8;

		void initialize()
		{
			static int dummy = []()
			{
				SteamNetworkingSockets_SetServiceThreadInitCallback(+[]() { currentThreadName("steam sockets"); });
				SteamNetworkingErrMsg msg;
				if (!GameNetworkingSockets_Init(nullptr, msg))
				{
					CAGE_LOG_THROW(msg);
					CAGE_THROW_ERROR(Exception, "failed to initialize steam networking library");
				}
				return 0;
			}();
		}

		CAGE_FORCE_INLINE ISteamNetworkingSockets *sockets()
		{
			return SteamNetworkingSockets();
		}

		CAGE_FORCE_INLINE ISteamNetworkingUtils *utils()
		{
			return SteamNetworkingUtils();
		}

		void handleResult(EResult r)
		{
			switch (r)
			{
				case k_EResultNone:
				case k_EResultOK:
					return;
				// todo handle individual errors
				default:
					CAGE_LOG_THROW(Stringizer() + "steam sockets result code: " + r);
					CAGE_THROW_ERROR(Exception, "steam sockets error");
			}
		}

		void statusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info);

		SteamNetworkingIPAddr parseAddress(const String &address, uint16 port)
		{
			auto al = privat::AddrList(address, port, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, 0);
			if (!al.valid())
			{
				CAGE_LOG_THROW(Stringizer() + "address: " + address);
				CAGE_THROW_ERROR(cage::Exception, "failed to convert address to ip");
			}
			SteamNetworkingIPAddr sa;
			if (al.family() == AF_INET6)
				sa.SetIPv6(endianness::change(al.address().getIp6()).data(), port);
			else
				sa.SetIPv4(*(uint32 *)al.address().getIp4().data(), port);
			return sa;
		}

		struct Test
		{
			bool testing()
			{
				if (!parseAddress("127.0.0.1", 1234).IsLocalHost())
					return false;
				if (!parseAddress("::1", 1234).IsLocalHost())
					return false;
				if (!parseAddress("localhost", 1234).IsLocalHost())
					return false;
				return true;
			}

			Test() { CAGE_ASSERT(testing()); }
		} testInstance;

		struct Data : public PointerRange<char>, private Immovable
		{
		private:
			SteamNetworkingMessage_t *msg = nullptr;

		public:
			Data(SteamNetworkingMessage_t *msg) : msg(msg) { ((PointerRange<char> &)*this) = { (char *)msg->m_pData, (char *)msg->m_pData + msg->m_cbSize }; }

			~Data()
			{
				if (msg)
					msg->Release();
			}
		};

		class GinnelConnectionImpl : public GinnelConnection
		{
		public:
			HSteamNetConnection sock = {};
			bool connected = false;
			bool disconnected = false;

			GinnelConnectionImpl(const String &address, uint16 port)
			{
				CAGE_ASSERT(((uint64)this % 2) == 0);
				SteamNetworkingConfigValue_t cfg[2] = {};
				cfg[0].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, (sint64)this);
				cfg[1].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)&statusChangedCallback);
				SteamNetworkingIPAddr sa = parseAddress(address, port);
				sock = sockets()->ConnectByIPAddress(sa, sizeof(cfg) / sizeof(cfg[0]), cfg);
				handleResult(sockets()->ConfigureConnectionLanes(sock, LanesCount, nullptr, nullptr));
			}

			GinnelConnectionImpl(HSteamNetConnection s) : sock(s)
			{
				CAGE_ASSERT(((uint64)this % 2) == 0);
				sockets()->SetConnectionUserData(sock, (sint64)this);
				handleResult(sockets()->AcceptConnection(sock));
				handleResult(sockets()->ConfigureConnectionLanes(sock, LanesCount, nullptr, nullptr));
			}

			~GinnelConnectionImpl() { sockets()->CloseConnection(sock, 0, nullptr, false); }

			Holder<PointerRange<char>> read(uint32 &channel, bool &reliable)
			{
				SteamNetworkingMessage_t *msg = nullptr;
				if (sockets()->ReceiveMessagesOnConnection(sock, &msg, 1) != 1)
					return {};
				channel = msg->m_idxLane;
				reliable = msg->m_nFlags & k_nSteamNetworkingSend_Reliable;
				return systemMemory().createImpl<PointerRange<char>, Data>(msg);
			}

			void write(PointerRange<const char> buffer, uint32 channel, bool reliable)
			{
				CAGE_ASSERT(channel < LanesCount);
				SteamNetworkingMessage_t *msg = utils()->AllocateMessage(buffer.size());
				detail::memcpy(msg->m_pData, buffer.data(), buffer.size());
				msg->m_conn = sock;
				msg->m_idxLane = channel;
				msg->m_nFlags = reliable ? k_nSteamNetworkingSend_Reliable : 0;
				sockets()->SendMessages(1, &msg, nullptr);
			}

			sint64 capacity() const
			{
				// todo
				return 0;
			}

			void update()
			{
				sockets()->RunCallbacks();
				if (disconnected)
				{
					detail::OverrideBreakpoint ob;
					CAGE_THROW_ERROR(Disconnected, "connection closed");
				}
				// todo
			}

			GinnelStatistics statistics() const
			{
				SteamNetConnectionRealTimeStatus_t r = {};
				handleResult(sockets()->GetConnectionRealTimeStatus(sock, &r, 0, nullptr));
				GinnelStatistics s;
				s.sendingBytesPerSecond = r.m_flOutBytesPerSec;
				s.receivingBytesPerSecond = r.m_flInBytesPerSec;
				s.estimatedBandwidth = r.m_nSendRateBytesPerSecond;
				s.ping = (uint64)r.m_nPing * 1000;
				s.quality = r.m_flConnectionQualityLocal < r.m_flConnectionQualityRemote ? r.m_flConnectionQualityLocal : r.m_flConnectionQualityRemote;
				return s;
			}
		};

		class GinnelServerImpl : public GinnelServer
		{
		public:
			HSteamListenSocket sock = {};
			ConcurrentQueue<HSteamNetConnection> waiting;

			GinnelServerImpl(uint16 port)
			{
				CAGE_ASSERT(((uint64)this % 2) == 0);
				SteamNetworkingConfigValue_t cfg[2] = {};
				cfg[0].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, (sint64)(sint64(this) + 1));
				cfg[1].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)&statusChangedCallback);
				SteamNetworkingIPAddr sa = {};
				sa.m_port = port;
				sock = sockets()->CreateListenSocketIP(sa, sizeof(cfg) / sizeof(cfg[0]), cfg);
			}

			~GinnelServerImpl()
			{
				/*
				while (true)
				{
					HSteamNetConnection c = 0;
					if (waiting.tryPop(c, true))
						sockets()->CloseConnection(c, 0, nullptr, false);
					else
						break;
				}
				*/
				sockets()->CloseListenSocket(sock);
			}

			Holder<GinnelConnection> accept()
			{
				sockets()->RunCallbacks();
				HSteamNetConnection c = 0;
				if (waiting.tryPop(c))
					return systemMemory().createImpl<GinnelConnection, GinnelConnectionImpl>(c);
				return {};
			}
		};

		GinnelConnectionImpl *getConnection(HSteamNetConnection sock)
		{
			sint64 v = sockets()->GetConnectionUserData(sock);
			CAGE_ASSERT(v >= 0);
			if ((v % 2) == 0)
				return (GinnelConnectionImpl *)v;
			return nullptr;
		}

		GinnelServerImpl *getServer(HSteamNetConnection sock)
		{
			sint64 v = sockets()->GetConnectionUserData(sock);
			CAGE_ASSERT(v >= 0);
			if ((v % 2) == 1)
				return (GinnelServerImpl *)(v - 1);
			return nullptr;
		}

		void statusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info)
		{
			if (info->m_info.m_hListenSocket && info->m_eOldState == k_ESteamNetworkingConnectionState_None)
			{
				CAGE_ASSERT(info->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting);
				if (GinnelServerImpl *impl = getServer(info->m_hConn))
					impl->waiting.push(info->m_hConn);
			}
			switch (info->m_info.m_eState)
			{
				case k_ESteamNetworkingConnectionState_Connected:
				{
					if (GinnelConnectionImpl *impl = getConnection(info->m_hConn))
						impl->connected = true;
					break;
				}
				case k_ESteamNetworkingConnectionState_ClosedByPeer:
				case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
				case k_ESteamNetworkingConnectionState_Dead:
				case k_ESteamNetworkingConnectionState_Linger:
				{
					if (GinnelConnectionImpl *impl = getConnection(info->m_hConn))
						impl->disconnected = true;
					break;
				}
			}
		}
	}

	Holder<PointerRange<char>> GinnelConnection::read(uint32 &channel, bool &reliable)
	{
		GinnelConnectionImpl *impl = (GinnelConnectionImpl *)this;
		return impl->read(channel, reliable);
	}

	Holder<PointerRange<char>> GinnelConnection::read()
	{
		uint32 channel;
		bool reliable;
		return read(channel, reliable);
	}

	void GinnelConnection::write(PointerRange<const char> buffer, uint32 channel, bool reliable)
	{
		GinnelConnectionImpl *impl = (GinnelConnectionImpl *)this;
		impl->write(buffer, channel, reliable);
	}

	sint64 GinnelConnection::capacity() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		return impl->capacity();
	}

	void GinnelConnection::update()
	{
		GinnelConnectionImpl *impl = (GinnelConnectionImpl *)this;
		impl->update();
	}

	GinnelStatistics GinnelConnection::statistics() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		return impl->statistics();
	}

	bool GinnelConnection::established() const
	{
		const GinnelConnectionImpl *impl = (const GinnelConnectionImpl *)this;
		return impl->connected;
	}

	Holder<GinnelConnection> GinnelServer::accept()
	{
		GinnelServerImpl *impl = (GinnelServerImpl *)this;
		return impl->accept();
	}

	Holder<GinnelConnection> newGinnelConnection(const String &address, uint16 port)
	{
		initialize();
		return systemMemory().createImpl<GinnelConnection, GinnelConnectionImpl>(address, port);
	}

	Holder<GinnelServer> newGinnelServer(uint16 port)
	{
		initialize();
		return systemMemory().createImpl<GinnelServer, GinnelServerImpl>(port);
	}
}
