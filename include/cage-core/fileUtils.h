#ifndef guard_fileutils_h_yesxrt92851637ojnuhg
#define guard_fileutils_h_yesxrt92851637ojnuhg

namespace cage
{
	class CAGE_API changeWatcherClass
	{
	public:
		void registerPath(const string &path);
		string waitForChange(uint64 time = m);
	};

	CAGE_API holder<changeWatcherClass> newChangeWatcher();

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

#endif // guard_fileutils_h_yesxrt92851637ojnuhg
