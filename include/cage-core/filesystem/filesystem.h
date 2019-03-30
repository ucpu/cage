namespace cage
{
	class CAGE_API filesystemClass
	{
	public:
		void changeDir(const string &path);
		string currentDir() const;
		pathTypeFlags type(const string &path) const;
		uint64 lastChange(const string &path) const;
		holder<fileClass> openFile(const string &path, const fileMode &mode);
		holder<directoryListClass> directoryList(const string &path);
		holder<changeWatcherClass> changeWatcher(const string &path);
		void move(const string &from, const string &to);
		void remove(const string &path);
	};

	CAGE_API holder<filesystemClass> newFilesystem();
}
