#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/files.h>
#include <cage-core/fileUtils.h>

#include <memory>

namespace cage
{
	class FileAbstract : public File
	{
	public:
		const string myPath; // full name as seen by the application
		FileMode mode;
		FileAbstract(const string &path, const FileMode &mode);
		virtual ~FileAbstract() {}
		virtual void read(void *data, uintPtr size) = 0;
		virtual void write(const void *data, uintPtr size) = 0;
		virtual void seek(uintPtr position) = 0;
		virtual void flush() = 0;
		virtual void close() = 0;
		virtual uintPtr tell() const = 0;
		virtual uintPtr size() const = 0;
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

	class ArchiveAbstract : public std::enable_shared_from_this<ArchiveAbstract>
	{
	public:
		const string myPath;
		ArchiveAbstract(const string &path);
		virtual ~ArchiveAbstract() {}
		virtual PathTypeFlags type(const string &path) = 0;
		virtual void createDirectories(const string &path) = 0;
		virtual void move(const string &from, const string &to) = 0;
		virtual void remove(const string &path) = 0;
		virtual uint64 lastChange(const string &path) = 0;
		virtual Holder<File> openFile(const string &path, const FileMode &mode) = 0;
		virtual Holder<DirectoryList> listDirectory(const string &path) = 0;
	};

	PathTypeFlags realType(const string &path);
	void realCreateDirectories(const string &path);
	void realMove(const string &from, const string &to);
	void realRemove(const string &path);
	uint64 realLastChange(const string &path);
	Holder<File> realNewFile(const string &path, const FileMode &mode);
	Holder<DirectoryList> realNewDirectoryList(const string &path);

	void mixedMove(std::shared_ptr<ArchiveAbstract> &af, const string &pf, std::shared_ptr<ArchiveAbstract> &at, const string &pt);
	std::shared_ptr<ArchiveAbstract> archiveFindTowardsRoot(const string &path, bool matchExact, string &insidePath);
	void archiveCreateZip(const string &path, const string &options);
	std::shared_ptr<ArchiveAbstract> archiveOpenZip(const string &path);
}
