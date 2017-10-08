namespace cage
{
	class CAGE_API directoryListClass
	{
	public:
		void next();
		holder<fileClass> openFile(const fileMode &mode);
		holder<filesystemClass> openDirectory();
		holder<directoryListClass> directoryList();
		bool valid() const;
		string name() const;
		bool isDirectory() const;
		uint64 lastChange() const;
	};

	CAGE_API holder<directoryListClass> newDirectoryList(const string &path);
}
