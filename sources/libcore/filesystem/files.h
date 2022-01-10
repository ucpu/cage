#ifndef guard_files_h_sdrgds45rfgt
#define guard_files_h_sdrgds45rfgt

#include <cage-core/files.h>

#include <memory>

namespace cage
{
	class FileAbstract : public File
	{
	public:
		const String myPath; // full name as seen by the application
		FileMode myMode;

		FileAbstract(const String &path, const FileMode &mode);

		virtual void reopenForModification();
		virtual void readAt(PointerRange<char> buffer, uintPtr at);
		FileMode mode() const override;
	};

	class DirectoryListAbstract : public DirectoryList
	{
	public:
		const String myPath;

		DirectoryListAbstract(const String &path);
		virtual ~DirectoryListAbstract() {}

		virtual bool valid() const = 0;
		virtual String name() const = 0;
		virtual void next() = 0;
		String fullPath() const;
	};

	class ArchiveAbstract : public std::enable_shared_from_this<ArchiveAbstract>, private Immovable
	{
	public:
		const String myPath;
		bool succesfullyMounted = false;

		ArchiveAbstract(const String &path);
		virtual ~ArchiveAbstract();

		virtual PathTypeFlags type(const String &path) const = 0;
		virtual void createDirectories(const String &path) = 0;
		virtual void move(const String &from, const String &to) = 0;
		virtual void remove(const String &path) = 0;
		virtual uint64 lastChange(const String &path) const = 0;
		virtual Holder<File> openFile(const String &path, const FileMode &mode) = 0;
		virtual Holder<DirectoryList> listDirectory(const String &path) const = 0;
	};

	void mixedMove(std::shared_ptr<ArchiveAbstract> &af, const String &pf, std::shared_ptr<ArchiveAbstract> &at, const String &pt);
	
	enum class ArchiveFindModeEnum
	{
		FileExclusiveThrow,
		ArchiveExclusiveThrow,
		ArchiveExclusiveNull,
		ArchiveShared,
	};
	struct ArchiveWithPath
	{
		std::shared_ptr<ArchiveAbstract> archive;
		String path;
	};
	ArchiveWithPath archiveFindTowardsRoot(const String &path, ArchiveFindModeEnum mode);

	void archiveCreateZip(const String &path, const String &options);
	std::shared_ptr<ArchiveAbstract> archiveOpenZip(Holder<File> &&f);
	std::shared_ptr<ArchiveAbstract> archiveOpenZipTry(Holder<File> &&f);

	std::shared_ptr<ArchiveAbstract> archiveOpenReal(const String &path);
}

#endif // guard_files_h_sdrgds45rfgt
