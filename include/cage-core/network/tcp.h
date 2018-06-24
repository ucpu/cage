namespace cage
{
	class CAGE_API tcpConnectionClass
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

	class CAGE_API tcpServerClass
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		holder<tcpConnectionClass> accept(); // non-blocking
	};

	CAGE_API holder<tcpServerClass> newTcpServer(uint16 port); // non-blocking
}
