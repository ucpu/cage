namespace cage
{
	class CAGE_API udpConnectionClass
	{
	public:
		uintPtr available() const;
		void read(void *buffer, uintPtr size, uint32 &channel);
		void read(void *buffer, uintPtr size);
		memoryBuffer read(uint32 &channel);
		memoryBuffer read();
		void write(const void *buffer, uintPtr size, uint32 channel, bool reliable);
		void write(const memoryBuffer &buffer, uint32 channel, bool reliable);
	};

	class CAGE_API udpServerClass
	{
	public:
		holder<udpConnectionClass> accept();
	};

	CAGE_API holder<udpConnectionClass> newUdpConnection(const string &address, uint16 port, uint32 channels, uint64 timeout = 3000000);
	CAGE_API holder<udpServerClass> newUdpServer(uint16 port, uint32 channels, uint32 maxClients);
}
