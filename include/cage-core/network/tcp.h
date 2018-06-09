namespace cage
{
	class CAGE_API tcpConnectionClass
	{
	public:
		string address() const;
		uint16 port() const;

		uintPtr available() const;
		void read(void *buffer, uintPtr size); // may block
		memoryBuffer read(); // never blocks
		void write(void *buffer, uintPtr size);
		void write(const memoryBuffer &buffer);

		bool availableLine() const;
		bool readLine(string &line);
		void writeLine(const string &line);
	};

	class CAGE_API tcpServerClass
	{
	public:
		uint16 port() const;

		holder<tcpConnectionClass> accept();
	};

	CAGE_API holder<tcpConnectionClass> newTcpConnection(const string &address, uint16 port);
	CAGE_API holder<tcpServerClass> newTcpServer(uint16 port);
}
