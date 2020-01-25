#ifndef guard_fileutils_h_yesxrt92851637ojnuhg
#define guard_fileutils_h_yesxrt92851637ojnuhg

namespace cage
{
	class CAGE_API FilesystemWatcher : private Immovable
	{
	public:
		void registerPath(const string &path);
		string waitForChange(uint64 time = m);
	};

	CAGE_API Holder<FilesystemWatcher> newFilesystemWatcher();

	class CAGE_API DirectoryList : private Immovable
	{
	public:
		bool valid() const;
		string name() const;
		PathTypeFlags type() const;
		bool isDirectory() const; // directory or archive
		uint64 lastChange() const;
		Holder<File> openFile(const FileMode &mode);
		Holder<Filesystem> openDirectory();
		Holder<DirectoryList> listDirectory();
		void next();
	};

	CAGE_API Holder<DirectoryList> newDirectoryList(const string &path);

	class [[deprecated]] CAGE_API Filesystem : private Immovable
	{
	public:
		void changeDir(const string &path);
		string currentDir() const;
		PathTypeFlags type(const string &path) const;
		uint64 lastChange(const string &path) const;
		Holder<File> openFile(const string &path, const FileMode &mode);
		Holder<DirectoryList> listDirectory(const string &path);
		Holder<FilesystemWatcher> watchFilesystem(const string &path);
		void move(const string &from, const string &to);
		void remove(const string &path);
	};

	[[deprecated]] CAGE_API Holder<Filesystem> newFilesystem();
}

#endif // guard_fileutils_h_yesxrt92851637ojnuhg
