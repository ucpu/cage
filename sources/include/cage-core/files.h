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
		void read(PointerRange<char> buffer);
		Holder<PointerRange<char>> read(uintPtr size);
		Holder<PointerRange<char>> readAll();
		bool readLine(string &line);

		void write(PointerRange<const char> buffer);
		void writeLine(const string &line);

		void seek(uintPtr position);
		uintPtr tell() const;
		uintPtr size() const;

		void close();

		FileMode mode() const;
	};

	CAGE_CORE_API Holder<File> newFile(const string &path, const FileMode &mode);
	CAGE_CORE_API Holder<File> readFile(const string &path);
	CAGE_CORE_API Holder<File> writeFile(const string &path);
	CAGE_CORE_API Holder<File> newFileBuffer(MemoryBuffer *buffer, const FileMode &mode = FileMode(true, true)); // the buffer must outlive the file
	CAGE_CORE_API Holder<File> newFileBuffer(MemoryBuffer &&buffer, const FileMode &mode = FileMode(true, true)); // the file takes ownership of the buffer
	CAGE_CORE_API Holder<File> newFileBuffer(PointerRange<char> buffer, const FileMode &mode = FileMode(true, false)); // the buffer must outlive the file
	CAGE_CORE_API Holder<File> newFileBuffer(PointerRange<const char> buffer); // the buffer must outlive the file
	CAGE_CORE_API Holder<File> newFileBuffer();

	class CAGE_CORE_API DirectoryList : private Immovable
	{
	public:
		bool valid() const;
		string name() const;
		string fullPath() const;
		PathTypeFlags type() const;
		bool isDirectory() const; // directory or archive
		uint64 lastChange() const;
		Holder<File> openFile(const FileMode &mode);
		Holder<File> readFile();
		Holder<File> writeFile();
		Holder<DirectoryList> listDirectory();
		void next();
	};

	CAGE_CORE_API Holder<DirectoryList> newDirectoryList(const string &path);

	// receive operating system issued notifications about filesystem changes
	// use registerPath to add a folder (and all of its subdirectories) to the watch list
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
	};
	GCHL_ENUM_BITS(PathTypeFlags);

	// string manipulation only (does not touch actual filesystem)

	CAGE_CORE_API bool pathIsValid(const string &path);
	CAGE_CORE_API bool pathIsAbs(const string &path);
	CAGE_CORE_API string pathToRel(const string &path, const string &ref = ""); // may return absolute path if it cannot be converted
	CAGE_CORE_API string pathToAbs(const string &path);
	CAGE_CORE_API string pathJoin(const string &a, const string &b);
	CAGE_CORE_API string pathJoinUnchecked(const string &a, const string &b); // joins paths with slash where needed, without validating the arguments
	CAGE_CORE_API string pathSimplify(const string &path);
	CAGE_CORE_API string pathReplaceInvalidCharacters(const string &path, const string &replacement = "_", bool allowDirectories = false);

	// path decomposition

	CAGE_CORE_API void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension);
	CAGE_CORE_API string pathExtractDrive(const string &input);
	CAGE_CORE_API string pathExtractDirectory(const string &input);
	CAGE_CORE_API string pathExtractDirectoryNoDrive(const string &input);
	CAGE_CORE_API string pathExtractFilename(const string &input);
	CAGE_CORE_API string pathExtractFilenameNoExtension(const string &input);
	CAGE_CORE_API string pathExtractExtension(const string &input);

	// filesystem manipulation

	CAGE_CORE_API PathTypeFlags pathType(const string &path);
	CAGE_CORE_API bool pathIsFile(const string &path);

	// create all parent folders for the entire path
	CAGE_CORE_API void pathCreateDirectories(const string &path);

	// create an empty archive at the specified path, it can be populated afterwards
	CAGE_CORE_API void pathCreateArchive(const string &path, const string &options = "");

	// moves (renames) a file or directory
	CAGE_CORE_API void pathMove(const string &from, const string &to);

	// permanently removes a file or folder including all sub-folders
	CAGE_CORE_API void pathRemove(const string &path);

	// path modification time is not related to any other source of time
	// the only valid operation is to compare it to repeated calls to pathLastChange
	CAGE_CORE_API uint64 pathLastChange(const string &path);

	// example, name = logo.png, whereToStart = /abc/def/ghi will look for files at these paths (in order):
	// /abc/def/ghi/logo.png
	// /abc/def/logo.png
	// /abc/logo.png
	// /logo.png
	// and throws an exception if none of the paths exists
	// if whereToStart is omitted, the search starts in current working directory
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
