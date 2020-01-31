#ifndef guard_filesystem_h_926e1c86_35dc_4d37_8333_e8a32f9b1ed1_
#define guard_filesystem_h_926e1c86_35dc_4d37_8333_e8a32f9b1ed1_

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API FileMode
	{
		explicit FileMode(bool read, bool write) : read(read), write(write)
		{}

		bool valid() const;
		string mode() const;

		bool read = false;
		bool write = false;
		bool textual = false;
		bool append = false;
	};

	class CAGE_CORE_API File : private Immovable
	{
	public:
		void read(void *data, uintPtr size);
		bool readLine(string &line);
		MemoryBuffer readBuffer(uintPtr size);
		void write(const void *data, uintPtr size);
		void writeLine(const string &line);
		void writeBuffer(const MemoryBuffer &buffer);
		void seek(uintPtr position);
		void flush();
		void close();
		uintPtr tell() const;
		uintPtr size() const;
	};

	CAGE_CORE_API Holder<File> newFile(const string &path, const FileMode &mode);

	class CAGE_CORE_API DirectoryList : private Immovable
	{
	public:
		bool valid() const;
		string name() const;
		PathTypeFlags type() const;
		bool isDirectory() const; // directory or archive
		uint64 lastChange() const;
		Holder<File> openFile(const FileMode &mode);
		Holder<DirectoryList> listDirectory();
		void next();
	};

	CAGE_CORE_API Holder<DirectoryList> newDirectoryList(const string &path);

	class CAGE_CORE_API FilesystemWatcher : private Immovable
	{
	public:
		void registerPath(const string &path);
		string waitForChange(uint64 time = m);
	};

	CAGE_CORE_API Holder<FilesystemWatcher> newFilesystemWatcher();

	enum class PathTypeFlags : uint32
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
	GCHL_ENUM_BITS(PathTypeFlags);

	// string manipulation only (does not touch any hdd)

	CAGE_CORE_API bool pathIsValid(const string &path);
	CAGE_CORE_API bool pathIsAbs(const string &path);
	CAGE_CORE_API string pathToRel(const string &path, const string &ref = "");
	CAGE_CORE_API string pathToAbs(const string &path);
	CAGE_CORE_API string pathJoin(const string &a, const string &b);
	CAGE_CORE_API string pathSimplify(const string &path);
	CAGE_CORE_API string pathReplaceInvalidCharacters(const string &path, const string &replacement = "_", bool allowDirectories = false);

	// path decomposition

	CAGE_CORE_API void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension);
	CAGE_CORE_API string pathExtractDrive(const string &input);
	CAGE_CORE_API string pathExtractPath(const string &input);
	CAGE_CORE_API string pathExtractPathNoDrive(const string &input);
	CAGE_CORE_API string pathExtractFilename(const string &input);
	CAGE_CORE_API string pathExtractFilenameNoExtension(const string &input);
	CAGE_CORE_API string pathExtractExtension(const string &input);

	// filesystem manipulation

	CAGE_CORE_API PathTypeFlags pathType(const string &path);
	CAGE_CORE_API bool pathIsFile(const string &path);
	CAGE_CORE_API void pathCreateDirectories(const string &path);
	CAGE_CORE_API void pathCreateArchive(const string &path, const string &options = "");
	CAGE_CORE_API void pathMove(const string &from, const string &to);
	CAGE_CORE_API void pathRemove(const string &path);
	CAGE_CORE_API uint64 pathLastChange(const string &path);
	CAGE_CORE_API string pathSearchTowardsRoot(const string &name, PathTypeFlags type = PathTypeFlags::File);
	CAGE_CORE_API string pathSearchTowardsRoot(const string &name, const string &whereToStart, PathTypeFlags type = PathTypeFlags::File);

	CAGE_CORE_API string pathWorkingDir();

	namespace detail
	{
		CAGE_CORE_API string getExecutableFullPath();
		CAGE_CORE_API string getExecutableFullPathNoExe();
	}
}

#endif // guard_filesystem_h_926e1c86_35dc_4d37_8333_e8a32f9b1ed1_
