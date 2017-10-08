namespace cage
{
	struct CAGE_API fileMode
	{
		fileMode(bool read, bool write, bool textual = false, bool append = false) : read(read), write(write), textual(textual), append(append)
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
		void write(const void *data, uint64 size);
		void writeLine(const string &line);
		void seek(uint64 position);
		void reopen(const fileMode &mode);
		void flush();
		void close();
		uint64 tell() const;
		uint64 size() const;
		string name() const;
		string path() const;
		fileMode mode() const;
	};

	CAGE_API holder<fileClass> newFile(const string &name, const fileMode &mode);
}
