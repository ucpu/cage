namespace cage
{
	enum class pathTypeFlags : uint32
	{
		None = 0,
		Invalid = 1 << 0,
		NotFound = 1 << 1,
		Directory = 1 << 2,
		File = 1 << 3,
		Archive = 1 << 4,
		InsideArchive = 1 << 5,
		Symlink = 1 << 6,
		InsideSymlink = 1 << 7,
	};
	GCHL_ENUM_BITS(pathTypeFlags);

	// string manipulation only (does not touch any hdd)

	CAGE_API bool pathIsValid(const string &path);
	CAGE_API bool pathIsAbs(const string &path);
	CAGE_API string pathToRel(const string &path, const string &ref = "");
	CAGE_API string pathToAbs(const string &path);
	CAGE_API string pathJoin(const string &a, const string &b);
	CAGE_API string pathSimplify(const string &path);
	CAGE_API string pathReplaceInvalidCharacters(const string &path, const string &replacement = "_", bool allowDirectories = false);

	// path decomposition

	CAGE_API void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension);
	CAGE_API string pathExtractDrive(const string &input);
	CAGE_API string pathExtractPath(const string &input);
	CAGE_API string pathExtractPathNoDrive(const string &input);
	CAGE_API string pathExtractFilename(const string &input);
	CAGE_API string pathExtractFilenameNoExtension(const string &input);
	CAGE_API string pathExtractExtension(const string &input);

	// filesystem manipulation

	CAGE_API pathTypeFlags pathType(const string &path);
	CAGE_API bool pathIsFile(const string &path);
	CAGE_API void pathCreateDirectories(const string &path);
	CAGE_API void pathCreateArchive(const string &path, const string &options = "");
	CAGE_API void pathMove(const string &from, const string &to);
	CAGE_API void pathRemove(const string &path);
	CAGE_API uint64 pathLastChange(const string &path);
	CAGE_API string pathSearchTowardsRoot(const string &name, pathTypeFlags type = pathTypeFlags::File);
	CAGE_API string pathSearchTowardsRoot(const string &name, const string &whereToStart, pathTypeFlags type = pathTypeFlags::File);

	CAGE_API string pathWorkingDir();

	namespace detail
	{
		CAGE_API string getExecutableFullPath();
		CAGE_API string getExecutableFullPathNoExe();
	}
}