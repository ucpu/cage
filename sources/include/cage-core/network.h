#ifndef guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_
#define guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API Disconnected : public Exception
	{
		using Exception::Exception;
	};

	// tcp

	class CAGE_CORE_API TcpConnection : private Immovable
	{
	public:
		string address() const; // remote address
		uint16 port() const; // remote port

		uintPtr available() const; // non-blocking

		void readWait(PointerRange<char> buffer);
		void read(PointerRange<char> &buffer);
		MemoryBuffer readWait(uintPtr size);
		MemoryBuffer read();
		string readLineWait();
		bool readLine(string &line);

		void write(PointerRange<const char> buffer);
		void writeLine(const string &line);
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
		uint64 roundTripDuration;
		uint64 bytesReceivedTotal, bytesSentTotal, bytesDeliveredTotal;
		uint64 bytesReceivedLately, bytesSentLately, bytesDeliveredLately;
		uint32 packetsReceivedTotal, packetsSentTotal, packetsDeliveredTotal;
		uint32 packetsReceivedLately, packetsSentLately, packetsDeliveredLately;

		UdpStatistics();

		// example: bytesPerSecondDeliveredAverage = 1000000 * bytesDeliveredLately / roundTripDuration;
	};

	// low latency, connection-oriented, sequenced and optionally reliable datagram protocol on top of udp
	class CAGE_CORE_API UdpConnection : private Immovable
	{
	public:
		// returns size of the first message queued for reading, if any, and zero otherwise
		uintPtr available();

		// reading throws an exception when nothing is available or the buffer is too small
		// you should call available first to determine if any messages are ready and what is the required buffer size
		void read(PointerRange<char> &buffer, uint32 &channel, bool &reliable);
		void read(PointerRange<char> &buffer);
		MemoryBuffer read(uint32 &channel, bool &reliable);
		MemoryBuffer read();

		void write(PointerRange<const char> buffer, uint32 channel, bool reliable);

		// update will check for received packets and flush packets pending for send
		// update also manages timeouts and resending, therefore it should be called periodically even if you wrote nothing
		void update();

		const UdpStatistics &statistics() const;
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
		void update();
		string message;
	};

	CAGE_CORE_API Holder<DiscoveryServer> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId);
}

#endif // guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_
