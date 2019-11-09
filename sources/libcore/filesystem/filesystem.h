#include <memory>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/files.h>
#include <cage-core/fileUtils.h>

namespace cage
{
	class fileVirtual : public fileHandle
	{
	public:
		const string myPath; // full name as seen by the application
		fileMode mode;
		fileVirtual(const string &path, const fileMode &mode);
		virtual void read(void *data, uintPtr size) = 0;
		virtual void write(const void *data, uintPtr size) = 0;
		virtual void seek(uintPtr position) = 0;
		virtual void flush() = 0;
		virtual void close() = 0;
		virtual uintPtr tell() const = 0;
		virtual uintPtr size() const = 0;
	};

	class directoryListVirtual : public directoryList
	{
	public:
		const string myPath;
		directoryListVirtual(const string &path);
		virtual bool valid() const = 0;
		virtual string name() const = 0;
		virtual void next() = 0;
		string fullPath() const;
	};

	class archiveVirtual : public std::enable_shared_from_this<archiveVirtual>
	{
	public:
		const string myPath;
		archiveVirtual(const string &path);
		virtual pathTypeFlags type(const string &path) = 0;
		virtual void createDirectories(const string &path) = 0;
		virtual void move(const string &from, const string &to) = 0;
		virtual void remove(const string &path) = 0;
		virtual uint64 lastChange(const string &path) = 0;
		virtual holder<fileHandle> openFile(const string &path, const fileMode &mode) = 0;
		virtual holder<directoryList> listDirectory(const string &path) = 0;
	};

	pathTypeFlags realType(const string &path);
	void realCreateDirectories(const string &path);
	void realMove(const string &from, const string &to);
	void realRemove(const string &path);
	uint64 realLastChange(const string &path);
	holder<fileHandle> realNewFile(const string &path, const fileMode &mode);
	holder<directoryList> realNewDirectoryList(const string &path);

	void mixedMove(std::shared_ptr<archiveVirtual> &af, const string &pf, std::shared_ptr<archiveVirtual> &at, const string &pt);
	std::shared_ptr<archiveVirtual> archiveFindTowardsRoot(const string &path, bool matchExact, string &insidePath);
	void archiveCreateZip(const string &path, const string &options);
	std::shared_ptr<archiveVirtual> archiveOpenZip(const string &path);
}
