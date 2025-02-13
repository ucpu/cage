#ifndef guard_files_h_sdrgds45rfgt
#define guard_files_h_sdrgds45rfgt

#include <memory>

#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	class FileAbstract : public File
	{
	public:
		const String myPath; // full name as seen by the application
		FileMode myMode;

		FileAbstract(const String &path, const FileMode &mode);

		virtual void reopenForModification();
		virtual void readAt(PointerRange<char> buffer, uint64 at);
		FileMode mode() const final;
	};

	class ArchiveAbstract : public std::enable_shared_from_this<ArchiveAbstract>, private Noncopyable
	{
	public:
		const String myPath;

		ArchiveAbstract(const String &path);
		ArchiveAbstract(ArchiveAbstract &&) = default;
		virtual ~ArchiveAbstract();

		virtual PathTypeFlags type(const String &path) const = 0;
		virtual PathLastChange lastChange(const String &path) const = 0;
		virtual Holder<PointerRange<String>> listDirectory(const String &path) const = 0;
		virtual void createDirectories(const String &path) = 0;
		virtual void move(const String &from, const String &to, bool copying) = 0;
		virtual void remove(const String &path) = 0;
		virtual Holder<File> openFile(const String &path, const FileMode &mode) = 0;
	};

	enum class ArchiveFindModeEnum
	{
		FileExclusive,
		ArchiveExclusive,
		ArchiveShared,
	};
	struct ArchiveWithPath
	{
		std::shared_ptr<ArchiveAbstract> archive;
		String path;
	};
	ArchiveWithPath archiveFindTowardsRoot(const String &path, ArchiveFindModeEnum mode);

	RecursiveMutex *fsMutex();

	Holder<File> newProxyFile(File *f, uint64 start, uint64 size);

	void writeZeroes(File *f, uint64 size);
}

#endif // guard_files_h_sdrgds45rfgt
