#ifndef guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_
#define guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_

namespace cage
{
	struct CAGE_API disconnectedException : public exception
	{
		disconnectedException(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept;
	};

	// tcp

	class CAGE_API tcpConnectionClass : private immovable
	{
	public:
		string address() const; // remote address
		uint16 port() const; // remote port

		uintPtr available() const; // non-blocking
		void read(void *buffer, uintPtr size); // blocking
		memoryBuffer read(uintPtr size); // blocking
		memoryBuffer read(); // non-blocking
		void write(const void *buffer, uintPtr size);
		void write(const memoryBuffer &buffer);

		bool readLine(string &line); // non-blocking
		void writeLine(const string &line);
	};

	CAGE_API holder<tcpConnectionClass> newTcpConnection(const string &address, uint16 port); // blocking

	class CAGE_API tcpServerClass : private immovable
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		holder<tcpConnectionClass> accept(); // non-blocking
	};

	CAGE_API holder<tcpServerClass> newTcpServer(uint16 port); // non-blocking

	// udp

	struct CAGE_API udpConnectionStatisticsStruct
	{
		uint64 roundTripDuration;
		uint64 bytesReceivedTotal, bytesSentTotal, bytesDeliveredTotal;
		uint64 bytesReceivedLately, bytesSentLately, bytesDeliveredLately;
		uint32 packetsReceivedTotal, packetsSentTotal, packetsDeliveredTotal;
		uint32 packetsReceivedLately, packetsSentLately, packetsDeliveredLately;

		udpConnectionStatisticsStruct();

		// example: bytesPerSecondDeliveredAverage = 1000000 * bytesDeliveredLately / roundTripDuration;
	};

	// low latency, connection-oriented, sequenced and optionally reliable datagram protocol on top of udp
	class CAGE_API udpConnectionClass : private immovable
	{
	public:
		// returns size of the first packet queued for reading, if any, and zero otherwise
		uintPtr available();

		// reading throws an exception when nothing is available or the buffer is too small
		// you should call available first to determine if any packets are ready and what is the required buffer size
		uintPtr read(void *buffer, uintPtr maxSize, uint32 &channel, bool &reliable);
		uintPtr read(void *buffer, uintPtr maxSize);
		memoryBuffer read(uint32 &channel, bool &reliable);
		memoryBuffer read();

		void write(const void *buffer, uintPtr size, uint32 channel, bool reliable);
		void write(const memoryBuffer &buffer, uint32 channel, bool reliable);

		// update will check for received packets and flush packets pending for send
		// update also manages timeouts and resending, therefore it should be called periodically even if you wrote nothing
		void update();

		const udpConnectionStatisticsStruct &statistics() const;
	};

	// non-zero timeout will block the caller for up to the specified time to ensure that the connection is established and throw an exception otherwise
	// zero timeout will return immediately and the connection will be established progressively as you use it
	CAGE_API holder<udpConnectionClass> newUdpConnection(const string &address, uint16 port, uint64 timeout = 3000000);

	class CAGE_API udpServerClass : private immovable
	{
	public:
		// returns empty holder if no new peer has connected
		holder<udpConnectionClass> accept(); // non-blocking
	};

	CAGE_API holder<udpServerClass> newUdpServer(uint16 port); // non-blocking

	// discovery

	struct CAGE_API discoveryPeerStruct
	{
		string message;
		string address;
		uint16 port;

		discoveryPeerStruct();
	};

	class CAGE_API discoveryClientClass : private immovable
	{
	public:
		void update();
		void addServer(const string &address, uint16 port);
		uint32 peersCount() const;
		discoveryPeerStruct peerData(uint32 index) const;
		holder<pointerRange<discoveryPeerStruct>> peers() const;
	};

	class CAGE_API discoveryServerClass : private immovable
	{
	public:
		void update();
		string message;
	};

	CAGE_API holder<discoveryClientClass> newDiscoveryClient(uint16 listenPort, uint32 gameId);
	CAGE_API holder<discoveryServerClass> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId);
}

#endif // guard_network_h_f311f15d_426e_4077_8b9b_a5ee12e78b39_
