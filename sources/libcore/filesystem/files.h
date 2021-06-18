#ifndef guard_files_h_sdrgds45rfgt
#define guard_files_h_sdrgds45rfgt

#include <cage-core/files.h>

#include <memory>

namespace cage
{
	class FileAbstract : public File
	{
	public:
		const string myPath; // full name as seen by the application
		FileMode myMode;

		FileAbstract(const string &path, const FileMode &mode);

		virtual void reopenForModification();
		virtual void readAt(PointerRange<char> buffer, uintPtr at);
		FileMode mode() const override;
	};

	class DirectoryListAbstract : public DirectoryList
	{
	public:
		const string myPath;

		DirectoryListAbstract(const string &path);
		virtual ~DirectoryListAbstract() {}

		virtual bool valid() const = 0;
		virtual string name() const = 0;
		virtual void next() = 0;
		string fullPath() const;
	};

	class ArchiveAbstract : public std::enable_shared_from_this<ArchiveAbstract>, private Immovable
	{
	public:
		const string myPath;
		bool succesfullyMounted = false;

		ArchiveAbstract(const string &path);
		virtual ~ArchiveAbstract();

		virtual PathTypeFlags type(const string &path) const = 0;
		virtual void createDirectories(const string &path) = 0;
		virtual void move(const string &from, const string &to) = 0;
		virtual void remove(const string &path) = 0;
		virtual uint64 lastChange(const string &path) const = 0;
		virtual Holder<File> openFile(const string &path, const FileMode &mode) = 0;
		virtual Holder<DirectoryList> listDirectory(const string &path) const = 0;
	};

	void mixedMove(std::shared_ptr<ArchiveAbstract> &af, const string &pf, std::shared_ptr<ArchiveAbstract> &at, const string &pt);
	
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
		string path;
	};
	ArchiveWithPath archiveFindTowardsRoot(const string &path, ArchiveFindModeEnum mode);

	void archiveCreateZip(const string &path, const string &options);
	std::shared_ptr<ArchiveAbstract> archiveOpenZip(Holder<File> &&f);

	std::shared_ptr<ArchiveAbstract> archiveOpenReal(const string &path);
}

#endif // guard_files_h_sdrgds45rfgt
