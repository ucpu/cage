#ifndef guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_
#define guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_

#include "files.h"

namespace cage
{
	struct CAGE_CORE_API Disconnected : public Exception
	{
		using Exception::Exception;
	};

	// tcp

	class CAGE_CORE_API TcpConnection : public File
	{
	public:
		string address() const; // remote address
		uint16 port() const; // remote port
	};

	CAGE_CORE_API Holder<TcpConnection> newTcpConnection(const string &address, uint16 port); // blocking

	class CAGE_CORE_API TcpServer : private Immovable
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		Holder<TcpConnection> accept(); // non-blocking
	};

	CAGE_CORE_API Holder<TcpServer> newTcpServer(uint16 port); // non-blocking

	// udp

	struct CAGE_CORE_API UdpStatistics
	{
		uint64 timestamp = 0;
		uint64 roundTripDuration = 0;
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

	// low latency, connection-oriented, sequenced and optionally reliable datagram protocol on top of udp
	// messages are sequenced within each channel only; reliable and unreliable messages are not sequenced with each other
	class CAGE_CORE_API UdpConnection : private Immovable
	{
	public:
		// size of the first message queued for reading, if any, and zero otherwise
		uintPtr available();

		// reading throws an exception when nothing is available or the buffer is too small
		// you should call available first to determine if any messages are ready and what is the required buffer size
		void read(PointerRange<char> &buffer, uint32 &channel, bool &reliable);
		void read(PointerRange<char> &buffer);
		Holder<PointerRange<char>> read(uint32 &channel, bool &reliable);
		Holder<PointerRange<char>> read();

		void write(PointerRange<const char> buffer, uint32 channel, bool reliable);

		// update will check for received packets and flush packets pending for send
		// update also manages timeouts and resending, therefore it should be called periodically even if you wrote nothing
		void update();

		const UdpStatistics &statistics() const;

		// estimated transmission bandwidth in bytes per second
		// (it does not increase if you do not write enough data)
		uint64 bandwidth() const;

		// suggested capacity for writing, in bytes, before calling update, assuming the update is called at regular rate
		sint64 capacity() const;
	};

	// non-zero timeout will block the caller for up to the specified time to ensure that the connection is established and throw an exception otherwise
	// zero timeout will return immediately and the connection will be established progressively as you use it
	CAGE_CORE_API Holder<UdpConnection> newUdpConnection(const string &address, uint16 port, uint64 timeout);

	class CAGE_CORE_API UdpServer : private Immovable
	{
	public:
		// returns empty holder if no new peer has connected
		Holder<UdpConnection> accept(); // non-blocking
	};

	CAGE_CORE_API Holder<UdpServer> newUdpServer(uint16 port); // non-blocking

	// discovery

	struct CAGE_CORE_API DiscoveryPeer
	{
		string message;
		string address;
		uint16 port = 0;
	};

	class CAGE_CORE_API DiscoveryClient : private Immovable
	{
	public:
		void update();
		void addServer(const string &address, uint16 port);
		uint32 peersCount() const;
		DiscoveryPeer peerData(uint32 index) const;
		Holder<PointerRange<DiscoveryPeer>> peers() const;
	};

	CAGE_CORE_API Holder<DiscoveryClient> newDiscoveryClient(uint16 listenPort, uint32 gameId);

	class CAGE_CORE_API DiscoveryServer : private Immovable
	{
	public:
		string message;

		void update();
	};

	CAGE_CORE_API Holder<DiscoveryServer> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId);
}

#endif // guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_
