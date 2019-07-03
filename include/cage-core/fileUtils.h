#ifndef guard_fileutils_h_yesxrt92851637ojnuhg
#define guard_fileutils_h_yesxrt92851637ojnuhg

namespace cage
{
	class CAGE_API filesystemWatcher : private immovable
	{
	public:
		void registerPath(const string &path);
		string waitForChange(uint64 time = m);
	};

	CAGE_API holder<filesystemWatcher> newFilesystemWatcher();

	class CAGE_API directoryList : private immovable
	{
	public:
		bool valid() const;
		string name() const;
		pathTypeFlags type() const;
		bool isDirectory() const; // directory or archive
		uint64 lastChange() const;
		holder<fileHandle> openFile(const fileMode &mode);
		holder<filesystem> openDirectory();
		holder<directoryList> listDirectory();
		void next();
	};

	CAGE_API holder<directoryList> newDirectoryList(const string &path);

	class CAGE_API filesystem : private immovable
	{
	public:
		void changeDir(const string &path);
		string currentDir() const;
		pathTypeFlags type(const string &path) const;
		uint64 lastChange(const string &path) const;
		holder<fileHandle> openFile(const string &path, const fileMode &mode);
		holder<directoryList> listDirectory(const string &path);
		holder<filesystemWatcher> watchFilesystem(const string &path);
		void move(const string &from, const string &to);
		void remove(const string &path);
	};

	CAGE_API holder<filesystem> newFilesystem();
}

#endif // guard_fileutils_h_yesxrt92851637ojnuhg
