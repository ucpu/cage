#ifndef guard_filesystem_h_926e1c86_35dc_4d37_8333_e8a32f9b1ed1_
#define guard_filesystem_h_926e1c86_35dc_4d37_8333_e8a32f9b1ed1_

#include <cage-core/enumBits.h>

namespace cage
{
	struct MemoryBuffer;

	// string manipulation only (does not touch actual filesystem)

	CAGE_CORE_API bool pathIsValid(const String &path);
	CAGE_CORE_API bool pathIsAbs(const String &path);
	CAGE_CORE_API String pathToRel(const String &path, const String &ref = ""); // may return absolute path if it cannot be converted
	CAGE_CORE_API String pathToAbs(const String &path);
	CAGE_CORE_API String pathJoin(const String &a, const String &b);
	CAGE_CORE_API String pathJoinUnchecked(const String &a, const String &b); // joins paths with slash where needed, without validating the arguments
	CAGE_CORE_API String pathSimplify(const String &path);
	CAGE_CORE_API String pathReplaceInvalidCharacters(const String &path, const String &replacement = "_", bool allowDirectories = false);

	// path decomposition

	CAGE_CORE_API void pathDecompose(const String &input, String &drive, String &directory, String &file, String &extension);
	CAGE_CORE_API String pathExtractDrive(const String &input);
	CAGE_CORE_API String pathExtractDirectory(const String &input);
	CAGE_CORE_API String pathExtractDirectoryNoDrive(const String &input);
	CAGE_CORE_API String pathExtractFilename(const String &input);
	CAGE_CORE_API String pathExtractFilenameNoExtension(const String &input);
	CAGE_CORE_API String pathExtractExtension(const String &input);

	// file

	struct CAGE_CORE_API FileMode
	{
		explicit FileMode(bool read, bool write) : read(read), write(write) {}

		bool valid() const;
		String mode() const;

		bool read = false;
		bool write = false;
		bool textual = false;
		bool append = false;
	};

	class CAGE_CORE_API File : private Immovable
	{
	public:
		virtual void read(PointerRange<char> buffer);
		virtual Holder<PointerRange<char>> read(uintPtr size);
		virtual Holder<PointerRange<char>> readAll();
		virtual String readLine(); // may block or return empty string
		virtual bool readLine(String &line); // non blocking

		virtual void write(PointerRange<const char> buffer);
		virtual void writeLine(const String &line);

		virtual void seek(uintPtr position);
		virtual uintPtr tell();
		virtual uintPtr size();
		virtual FileMode mode() const;

		virtual void close();
		virtual ~File() = default;
	};

	CAGE_CORE_API Holder<File> newFile(const String &path, FileMode mode);
	CAGE_CORE_API Holder<File> readFile(const String &path);
	CAGE_CORE_API Holder<File> writeFile(const String &path);
	CAGE_CORE_API Holder<File> newFileBuffer(Holder<PointerRange<const char>> buffer);
	CAGE_CORE_API Holder<File> newFileBuffer(Holder<PointerRange<char>> buffer, FileMode mode = FileMode(true, false));
	CAGE_CORE_API Holder<File> newFileBuffer(Holder<const MemoryBuffer> buffer);
	CAGE_CORE_API Holder<File> newFileBuffer(Holder<MemoryBuffer> buffer, FileMode mode = FileMode(true, true));
	CAGE_CORE_API Holder<File> newFileBuffer(MemoryBuffer &&buffer, FileMode mode = FileMode(true, true));
	CAGE_CORE_API Holder<File> newFileBuffer();

	// receive operating system issued notifications about filesystem changes
	// use registerPath to add a folder (and all of its subdirectories) to the watch list
	class CAGE_CORE_API FilesystemWatcher : private Immovable
	{
	public:
		void registerPath(const String &path);
		String waitForChange(uint64 time = m);
	};

	CAGE_CORE_API Holder<FilesystemWatcher> newFilesystemWatcher();

	// filesystem manipulation

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

	CAGE_CORE_API PathTypeFlags pathType(const String &path);
	CAGE_CORE_API bool pathIsFile(const String &path);
	CAGE_CORE_API bool pathIsDirectory(const String &path); // directory or archive

	// path modification time is not related to any other source of time
	// the only valid operation is to compare it to repeated calls to pathLastChange
	struct CAGE_CORE_API PathLastChange
	{
		uint64 data = 0;
		auto operator<=>(const PathLastChange &) const = default;
		bool operator==(const PathLastChange &) const = default;
	};
	CAGE_CORE_API PathLastChange pathLastChange(const String &path);

	// enumerate all files and folders in a directory
	CAGE_CORE_API Holder<PointerRange<String>> pathListDirectory(const String &path);

	// example, name = logo.png, whereToStart = /abc/def/ghi will look for files at these paths (in order):
	// /abc/def/ghi/logo.png
	// /abc/def/logo.png
	// /abc/logo.png
	// /logo.png
	// and throws an exception if none of the paths exists
	// if whereToStart is omitted, the search starts in current working directory
	CAGE_CORE_API String pathSearchTowardsRoot(const String &name, PathTypeFlags type = PathTypeFlags::File);
	CAGE_CORE_API String pathSearchTowardsRoot(const String &name, const String &whereToStart, PathTypeFlags type = PathTypeFlags::File);

	// example, pattern = "/abc/def$$$.txt" will search for files:
	// /abc/def000.txt (optional)
	// /abc/def001.txt
	// /abc/def002.txt etc.
	// and stops searching on first path that does not exist (except all zeros)
	CAGE_CORE_API Holder<PointerRange<String>> pathSearchSequence(const String &pattern, char substitute = '$', uint32 limit = 1000);

	// create all parent folders for the entire path
	CAGE_CORE_API void pathCreateDirectories(const String &path);

	// create an empty archive at the specified path, it can be populated afterwards
	CAGE_CORE_API void pathCreateArchiveZip(const String &path);
	CAGE_CORE_API void pathCreateArchiveCarch(const String &path);

	// moves (renames) or copies a file or directory
	CAGE_CORE_API void pathMove(const String &from, const String &to);
	CAGE_CORE_API void pathCopy(const String &from, const String &to);

	// permanently removes a file or folder including all sub-folders
	CAGE_CORE_API void pathRemove(const String &path);

	CAGE_CORE_API String pathWorkingDir();

	namespace detail
	{
		CAGE_CORE_API String executableFullPath();
		CAGE_CORE_API String executableFullPathNoExe();

		// open a path in an archive preventing closing it to optimize bulk operations
		CAGE_CORE_API Holder<void> pathKeepOpen(const String &path);
	}
}

#endif // guard_filesystem_h_926e1c86_35dc_4d37_8333_e8a32f9b1ed1_
