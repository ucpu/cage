#ifndef guard_networkSteam_h_vrdfk4q
#define guard_networkSteam_h_vrdfk4q

#include <cage-core/networkUtils.h>

namespace cage
{
	struct CAGE_CORE_API SteamStatistics
	{
		uint64 sendingBytesPerSecond = 0;
		uint64 receivingBytesPerSecond = 0;
		uint64 estimatedBandwidth = 0; // bytes per second
		uint64 ping = 0;
		float quality = 0; // 0 = bad, 1 = good
	};

	struct CAGE_CORE_API SteamRemoteInfo
	{
		String address;
		uint64 steamUserId = 0;
		uint16 port = 0;
	};

	class CAGE_CORE_API SteamConnection : private Immovable
	{
	public:
		// returns empty holder if no message was available
		Holder<PointerRange<char>> read(uint32 &channel, bool &reliable);
		Holder<PointerRange<char>> read();

		void write(PointerRange<const char> buffer, uint32 channel, bool reliable);

		// suggested capacity for writing, in bytes
		sint64 capacity() const;

		// update will check for received packets and flush packets pending for send
		// update also manages timeouts and resending, therefore it should be called periodically even if you wrote nothing
		void update();

		SteamStatistics statistics() const;

		// successfully completed connection initialization
		bool established() const;

		SteamRemoteInfo remoteInfo() const;
	};

	CAGE_CORE_API Holder<SteamConnection> newSteamConnection(const String &address, uint16 port);
	CAGE_CORE_API Holder<SteamConnection> newSteamConnection(uint64 steamId); // requires steam sdk

	class CAGE_CORE_API SteamServer : private Immovable
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		// serves as an update, therefore it should be called periodically, even if you do not expect any new connections
		Holder<SteamConnection> accept();
	};

	struct CAGE_CORE_API SteamServerCreateConfig
	{
		uint16 port = 0;
		bool listenNetwork = true;
		bool listenSteamRelay = false; // requires steam sdk
	};

	CAGE_CORE_API Holder<SteamServer> newSteamServer(const SteamServerCreateConfig &config);
}

#endif // guard_networkSteam_h_vrdfk4q
