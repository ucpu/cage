#if defined(CAGE_USE_STEAM_SOCKETS) || defined(CAGE_USE_STEAM_SDK)

	#if defined(CAGE_USE_STEAM_SOCKETS)
		#include <steam/isteamnetworkingsockets.h>
		#include <steam/isteamnetworkingutils.h>
		#include <steam/steamnetworkingsockets.h>
	#endif
	#if defined(CAGE_USE_STEAM_SDK)
		#include <steam/steam_api.h>
	#endif

	#include "net.h"

	#include <cage-core/concurrent.h>
	#include <cage-core/concurrentQueue.h>
	#include <cage-core/endianness.h>
	#include <cage-core/networkSteam.h>
	#include <cage-core/config.h>

namespace cage
{
	namespace
	{
		const ConfigSint32 confDebugLogLevel("cage/steamsocks/logLevel", k_ESteamNetworkingSocketsDebugOutputType_Warning);

		constexpr uint32 LanesCount = 8;

		CAGE_FORCE_INLINE ISteamNetworkingSockets *sockets()
		{
			return SteamNetworkingSockets();
		}

		CAGE_FORCE_INLINE ISteamNetworkingUtils *utils()
		{
			return SteamNetworkingUtils();
		}

		const char *connectionStateToString(ESteamNetworkingConnectionState s)
		{
			switch (s)
			{
				case k_ESteamNetworkingConnectionState_None:
					return "none";
				case k_ESteamNetworkingConnectionState_Connecting:
					return "connecting";
				case k_ESteamNetworkingConnectionState_FindingRoute:
					return "finding route";
				case k_ESteamNetworkingConnectionState_Connected:
					return "connected";
				case k_ESteamNetworkingConnectionState_ClosedByPeer:
					return "closed by peer";
				case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
					return "problem detected locally";
				case k_ESteamNetworkingConnectionState_FinWait:
					return "fin wait";
				case k_ESteamNetworkingConnectionState_Linger:
					return "linger";
				case k_ESteamNetworkingConnectionState_Dead:
					return "dead";
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid steam networking connection state");
			}
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

		void debugOutputHandler(ESteamNetworkingSocketsDebugOutputType nType, const char *pszMsg)
		{
			const cage::SeverityEnum level = [&]() -> cage::SeverityEnum
			{
				switch (nType)
				{
					default:
					case k_ESteamNetworkingSocketsDebugOutputType_None:
					case k_ESteamNetworkingSocketsDebugOutputType_Bug:
						return SeverityEnum::Critical;
					case k_ESteamNetworkingSocketsDebugOutputType_Error:
						return SeverityEnum::Error;
					case k_ESteamNetworkingSocketsDebugOutputType_Important:
					case k_ESteamNetworkingSocketsDebugOutputType_Warning:
						return SeverityEnum::Warning;
					case k_ESteamNetworkingSocketsDebugOutputType_Msg:
						return SeverityEnum::Info;
					case k_ESteamNetworkingSocketsDebugOutputType_Verbose:
					case k_ESteamNetworkingSocketsDebugOutputType_Debug:
					case k_ESteamNetworkingSocketsDebugOutputType_Everything:
						return SeverityEnum::Note;
				}
			}();
			CAGE_LOG(level, "steamsocks", pszMsg);
		}

		void initializeSockets()
		{
	#if defined(CAGE_USE_STEAM_SOCKETS)
			SteamNetworkingSockets_SetServiceThreadInitCallback(+[]() { currentThreadName("steam sockets"); });
			SteamNetworkingErrMsg msg;
			if (!GameNetworkingSockets_Init(nullptr, msg))
			{
				CAGE_LOG_THROW(msg);
				CAGE_THROW_ERROR(Exception, "failed to initialize steam sockets networking library");
			}
	#endif
		}

		void initializeAuthentication()
		{
	#if defined(CAGE_USE_STEAM_SDK)
			ESteamNetworkingAvailability a = sockets()->InitAuthentication();
			while (true)
			{
				switch (a)
				{
					case k_ESteamNetworkingAvailability_Current:
						return; // all is done
					case k_ESteamNetworkingAvailability_CannotTry:
					case k_ESteamNetworkingAvailability_Failed:
					case k_ESteamNetworkingAvailability_Previously:
						CAGE_THROW_ERROR(Exception, "failed to initialize steam sockets network authentication");
						break;
					default:
						break;
				}
				threadSleep(5000);
				SteamAPI_RunCallbacks();
				a = sockets()->GetAuthenticationStatus(nullptr);
			}
	#endif
		}

		void initialize()
		{
			static int dummy = []()
			{
				initializeSockets();
				utils()->SetDebugOutputFunction((ESteamNetworkingSocketsDebugOutputType)(sint32)confDebugLogLevel, &debugOutputHandler);
				initializeAuthentication();
				return 0;
			}();
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

		class SteamConnectionImpl : public SteamConnection
		{
		public:
			HSteamNetConnection sock = {};
			bool connected = false;
			bool disconnected = false;

			SteamConnectionImpl(const String &address, uint16 port)
			{
				CAGE_ASSERT(((uint64)this % 2) == 0);
				SteamNetworkingConfigValue_t cfg[2] = {};
				cfg[0].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, (sint64)this);
				cfg[1].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)&statusChangedCallback);
				SteamNetworkingIPAddr sa = parseAddress(address, port);
				sock = sockets()->ConnectByIPAddress(sa, sizeof(cfg) / sizeof(cfg[0]), cfg);
				handleResult(sockets()->ConfigureConnectionLanes(sock, LanesCount, nullptr, nullptr));
			}

			SteamConnectionImpl(HSteamNetConnection s) : sock(s)
			{
				CAGE_ASSERT(((uint64)this % 2) == 0);
				sockets()->SetConnectionUserData(sock, (sint64)this);
				handleResult(sockets()->AcceptConnection(sock));
				handleResult(sockets()->ConfigureConnectionLanes(sock, LanesCount, nullptr, nullptr));
			}

			~SteamConnectionImpl() { sockets()->CloseConnection(sock, 0, nullptr, false); }

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

			SteamStatistics statistics() const
			{
				SteamNetConnectionRealTimeStatus_t r = {};
				handleResult(sockets()->GetConnectionRealTimeStatus(sock, &r, 0, nullptr));
				SteamStatistics s;
				s.sendingBytesPerSecond = r.m_flOutBytesPerSec;
				s.receivingBytesPerSecond = r.m_flInBytesPerSec;
				s.estimatedBandwidth = r.m_nSendRateBytesPerSecond;
				s.ping = (uint64)r.m_nPing * 1000;
				s.quality = r.m_flConnectionQualityLocal < r.m_flConnectionQualityRemote ? r.m_flConnectionQualityLocal : r.m_flConnectionQualityRemote;
				return s;
			}
		};

		class SteamServerImpl : public SteamServer
		{
		public:
			HSteamListenSocket sock = {};
			ConcurrentQueue<HSteamNetConnection> waiting;

			SteamServerImpl(uint16 port)
			{
				CAGE_ASSERT(((uint64)this % 2) == 0);
				SteamNetworkingConfigValue_t cfg[2] = {};
				cfg[0].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, (sint64)(sint64(this) + 1));
				cfg[1].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)&statusChangedCallback);
				SteamNetworkingIPAddr sa = {};
				sa.m_port = port;
				sock = sockets()->CreateListenSocketIP(sa, sizeof(cfg) / sizeof(cfg[0]), cfg);
			}

			~SteamServerImpl()
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

			Holder<SteamConnection> accept()
			{
				sockets()->RunCallbacks();
				HSteamNetConnection c = 0;
				if (waiting.tryPop(c))
					return systemMemory().createImpl<SteamConnection, SteamConnectionImpl>(c);
				return {};
			}
		};

		SteamConnectionImpl *getConnection(HSteamNetConnection sock)
		{
			sint64 v = sockets()->GetConnectionUserData(sock);
			CAGE_ASSERT(v >= 0);
			if ((v % 2) == 0)
				return (SteamConnectionImpl *)v;
			return nullptr;
		}

		SteamServerImpl *getServer(HSteamNetConnection sock)
		{
			sint64 v = sockets()->GetConnectionUserData(sock);
			CAGE_ASSERT(v >= 0);
			if ((v % 2) == 1)
				return (SteamServerImpl *)(v - 1);
			return nullptr;
		}

		void statusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info)
		{
			CAGE_LOG_DEBUG(SeverityEnum::Info, "steamsocks", Stringizer() + "connection " + info->m_info.m_szConnectionDescription + " transitions from: " + connectionStateToString(info->m_eOldState) + ", to: " + connectionStateToString(info->m_info.m_eState));
			if (info->m_info.m_hListenSocket && info->m_eOldState == k_ESteamNetworkingConnectionState_None)
			{
				CAGE_ASSERT(info->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting);
				if (SteamServerImpl *impl = getServer(info->m_hConn))
					impl->waiting.push(info->m_hConn);
			}
			switch (info->m_info.m_eState)
			{
				case k_ESteamNetworkingConnectionState_Connected:
				{
					if (SteamConnectionImpl *impl = getConnection(info->m_hConn))
						impl->connected = true;
					break;
				}
				case k_ESteamNetworkingConnectionState_ClosedByPeer:
				case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
				{
					CAGE_LOG_DEBUG(SeverityEnum::Info, "steamsocks", Stringizer() + "connection " + info->m_info.m_szConnectionDescription + " terminates with code: " + info->m_info.m_eEndReason + ", explanation: " + info->m_info.m_szEndDebug);
					if (SteamConnectionImpl *impl = getConnection(info->m_hConn))
						impl->disconnected = true;
					break;
				}
			}
		}
	}

	Holder<PointerRange<char>> SteamConnection::read(uint32 &channel, bool &reliable)
	{
		SteamConnectionImpl *impl = (SteamConnectionImpl *)this;
		return impl->read(channel, reliable);
	}

	Holder<PointerRange<char>> SteamConnection::read()
	{
		uint32 channel;
		bool reliable;
		return read(channel, reliable);
	}

	void SteamConnection::write(PointerRange<const char> buffer, uint32 channel, bool reliable)
	{
		SteamConnectionImpl *impl = (SteamConnectionImpl *)this;
		impl->write(buffer, channel, reliable);
	}

	sint64 SteamConnection::capacity() const
	{
		const SteamConnectionImpl *impl = (const SteamConnectionImpl *)this;
		return impl->capacity();
	}

	void SteamConnection::update()
	{
		SteamConnectionImpl *impl = (SteamConnectionImpl *)this;
		impl->update();
	}

	SteamStatistics SteamConnection::statistics() const
	{
		const SteamConnectionImpl *impl = (const SteamConnectionImpl *)this;
		return impl->statistics();
	}

	bool SteamConnection::established() const
	{
		const SteamConnectionImpl *impl = (const SteamConnectionImpl *)this;
		return impl->connected;
	}

	Holder<SteamConnection> SteamServer::accept()
	{
		SteamServerImpl *impl = (SteamServerImpl *)this;
		return impl->accept();
	}

	Holder<SteamConnection> newSteamConnection(const String &address, uint16 port)
	{
		initialize();
		return systemMemory().createImpl<SteamConnection, SteamConnectionImpl>(address, port);
	}

	Holder<SteamServer> newSteamServer(uint16 port)
	{
		initialize();
		return systemMemory().createImpl<SteamServer, SteamServerImpl>(port);
	}
}

#endif // CAGE_USE_STEAM_SOCKETS
