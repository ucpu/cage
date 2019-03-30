namespace cage
{
	struct CAGE_API fileMode
	{
		fileMode(bool read, bool write) : read(read), write(write), textual(false), append(false)
		{}

		bool valid() const;
		string mode() const;

		bool read;
		bool write;
		bool textual;
		bool append;
	};

	class CAGE_API fileClass
	{
	public:
		void read(void *data, uint64 size);
		bool readLine(string &line);
		memoryBuffer readBuffer(uintPtr size);
		void write(const void *data, uint64 size);
		void writeLine(const string &line);
		void writeBuffer(const memoryBuffer &buffer);
		void seek(uint64 position);
		void flush();
		void close();
		uint64 tell() const;
		uint64 size() const;
	};

	CAGE_API holder<fileClass> newFile(const string &path, const fileMode &mode);
}
