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

	class ArchiveAbstract : public std::enable_shared_from_this<ArchiveAbstract>, private Immovable
	{
	public:
		const String myPath;
		bool succesfullyMounted = false;

		ArchiveAbstract(const String &path);
		virtual ~ArchiveAbstract();

		virtual PathTypeFlags type(const String &path) const = 0;
		virtual PathLastChange lastChange(const String &path) const = 0;
		virtual Holder<PointerRange<String>> listDirectory(const String &path) const = 0;
		virtual void createDirectories(const String &path) = 0;
		virtual void move(const String &from, const String &to) = 0;
		virtual void remove(const String &path) = 0;
		virtual Holder<File> openFile(const String &path, const FileMode &mode) = 0;
	};

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
}

#endif // guard_files_h_sdrgds45rfgt
