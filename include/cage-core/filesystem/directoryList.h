namespace cage
{
	class CAGE_API directoryListClass
	{
	public:
		bool valid() const;
		string name() const;
		pathTypeFlags type() const;
		bool isDirectory() const; // directory or archive
		uint64 lastChange() const;
		holder<fileClass> openFile(const fileMode &mode);
		holder<filesystemClass> openDirectory();
		holder<directoryListClass> directoryList();
		void next();
	};

	CAGE_API holder<directoryListClass> newDirectoryList(const string &path);
}
