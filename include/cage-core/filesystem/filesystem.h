namespace cage
{
	class CAGE_API filesystemClass
	{
	public:
		void changeDir(const string &path);
		holder<directoryListClass> directoryList(const string &path);
		holder<changeWatcherClass> changeWatcher(const string &path);
		holder<fileClass> openFile(const string &path, const fileMode &mode);
		void move(const string &from, const string &to);
		void remove(const string &path);
		string currentDir() const;
		bool exists(const string &path) const;
		bool isDirectory(const string &path) const;
		uint64 lastChange(const string &path) const;
	};

	CAGE_API holder<filesystemClass> newFilesystem();

	CAGE_API string pathWorkingDir();
	CAGE_API bool pathIsValid(const string &path);
	CAGE_API string pathToRel(const string &path, const string &ref = "");
	CAGE_API string pathToAbs(const string &path);
	CAGE_API bool pathIsAbs(const string &path);
	CAGE_API void pathCreateDirectories(const string &path);
	CAGE_API void pathCreateArchive(const string &path, const string &type = "");
	CAGE_API bool pathExists(const string &path);
	CAGE_API bool pathIsDirectory(const string &path);
	CAGE_API void pathMove(const string &from, const string &to);
	CAGE_API void pathRemove(const string &path);
	CAGE_API string pathJoin(const string &a, const string &b);
	CAGE_API uint64 pathLastChange(const string &path);
	CAGE_API string pathFind(const string &name);
	CAGE_API string pathFind(const string &name, const string &whereToStart);

	CAGE_API string pathSimplify(const string &path);
	CAGE_API string pathMakeValid(const string &path, const string &replacement = "_", bool allowDirectories = false);

	CAGE_API void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension);
	CAGE_API string pathExtractDrive(const string &input);
	CAGE_API string pathExtractPath(const string &input);
	CAGE_API string pathExtractPathNoDrive(const string &input);
	CAGE_API string pathExtractFilename(const string &input);
	CAGE_API string pathExtractFilenameNoExtension(const string &input);
	CAGE_API string pathExtractExtension(const string &input);

	namespace detail
	{
		CAGE_API string getExecutableFullPath();
		CAGE_API string getExecutableFullPathNoExe();
	}
}
