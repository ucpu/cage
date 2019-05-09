namespace cage
{
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
	class CAGE_API udpConnectionClass
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

	class CAGE_API udpServerClass
	{
	public:
		// returns empty holder if no new peer has connected
		holder<udpConnectionClass> accept(); // non-blocking
	};

	CAGE_API holder<udpServerClass> newUdpServer(uint16 port); // non-blocking
}
