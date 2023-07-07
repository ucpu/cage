#ifndef guard_networkGinnel_h_yxdrz748wq
#define guard_networkGinnel_h_yxdrz748wq

#include "networkUtils.h"

namespace cage
{
	// Ginnel - noun
	//   Northern English: A narrow passage between buildings; an alley.
	//   Origin: Early 17th century perhaps from French chenel ‘channel’.

	// low latency, connection-oriented, sequenced and optionally reliable datagram protocol on top of udp
	// messages are sequenced within each channel only; reliable and unreliable messages are not sequenced with each other

	struct CAGE_CORE_API GinnelStatistics
	{
		uint64 sendingBytesPerSecond = 0;
		uint64 receivingBytesPerSecond = 0;
		uint64 estimatedBandwidth = 0; // bytes per second
		uint64 ping = 0;
		float quality = 0; // 0 = bad, 1 = good
	};

	class CAGE_CORE_API GinnelConnection : private Immovable
	{
	public:
		// returns empty holder if no message was available
		Holder<PointerRange<char>> read(uint32 &channel, bool &reliable);
		Holder<PointerRange<char>> read();

		void write(PointerRange<const char> buffer, uint32 channel, bool reliable);

		// suggested capacity for writing, in bytes, before calling update, assuming the update is called at regular rate
		sint64 capacity() const;

		// update will check for received packets and flush packets pending for send
		// update also manages timeouts and resending, therefore it should be called periodically even if you wrote nothing
		void update();

		GinnelStatistics statistics() const;

		// successfully completed connection initialization
		bool established() const;
	};

	CAGE_CORE_API Holder<GinnelConnection> newGinnelConnection(const String &address, uint16 port);

	class CAGE_CORE_API GinnelServer : private Immovable
	{
	public:
		// returns empty holder if no new peer has connected
		// serves as an update, therefore it should be called periodically, even if you do not expect any new connections
		Holder<GinnelConnection> accept();
	};

	CAGE_CORE_API Holder<GinnelServer> newGinnelServer(uint16 port);
}

#endif // guard_networkGinnel_h_yxdrz748wq
