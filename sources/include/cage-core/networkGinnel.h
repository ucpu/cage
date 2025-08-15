#ifndef guard_networkGinnel_h_yxdrz748wq
#define guard_networkGinnel_h_yxdrz748wq

#include <cage-core/networkUtils.h>

namespace cage
{
	// Ginnel - noun
	//   Northern English: A narrow passage between buildings; an alley.
	//   Origin: Early 17th century perhaps from French chenel ‘channel’.

	// low latency, connection-oriented, sequenced, and optionally reliable, datagram protocol on top of udp
	// messages are sequenced within each channel only; reliable and unreliable messages are not sequenced with each other

	struct CAGE_CORE_API GinnelStatistics
	{
		uint64 timestamp = 0;
		uint64 roundTripDuration = 0;
		uint64 estimatedBandwidth = 0; // bytes per second
		uint64 bytesReceivedTotal = 0, bytesSentTotal = 0, bytesDeliveredTotal = 0;
		uint64 bytesReceivedLately = 0, bytesSentLately = 0, bytesDeliveredLately = 0;
		uint32 packetsReceivedTotal = 0, packetsSentTotal = 0, packetsDeliveredTotal = 0;
		uint32 packetsReceivedLately = 0, packetsSentLately = 0, packetsDeliveredLately = 0;

		// bytes per second
		uint64 bpsReceived() const;
		uint64 bpsSent() const;
		uint64 bpsDelivered() const;

		// packets per second
		uint64 ppsReceived() const;
		uint64 ppsSent() const;
		uint64 ppsDelivered() const;
	};

	struct CAGE_CORE_API GinnelRemoteInfo
	{
		String address;
		uint16 port = 0;
	};

	class CAGE_CORE_API GinnelConnection : private Immovable
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

		const GinnelStatistics &statistics() const;

		// successfully completed connection initialization
		bool established() const;

		GinnelRemoteInfo remoteInfo() const;
	};

	// non-zero timeout will block the caller for up to the specified time to ensure that the connection is established and throw an exception otherwise
	// zero timeout will return immediately and the connection will be established progressively as you use it
	CAGE_CORE_API Holder<GinnelConnection> newGinnelConnection(const String &address, uint16 port, uint64 timeout);

	class CAGE_CORE_API GinnelServer : private Immovable
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		Holder<GinnelConnection> accept(); // non-blocking
	};

	CAGE_CORE_API Holder<GinnelServer> newGinnelServer(uint16 port); // non-blocking
}

#endif // guard_networkGinnel_h_yxdrz748wq
