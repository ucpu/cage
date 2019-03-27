#include <memory>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/filesystem.h>

namespace cage
{
	class fileVirtual : public fileClass
	{
	public:
		const string name; // full name as seen by the application
		fileMode mode;
		fileVirtual(const string &name, const fileMode &mode);
		virtual void read(void *data, uint64 size) = 0;
		virtual void write(const void *data, uint64 size) = 0;
		virtual void seek(uint64 position) = 0;
		virtual void reopen(const fileMode &mode) = 0;
		virtual void flush() = 0;
		virtual void close() = 0;
		virtual uint64 tell() const = 0;
		virtual uint64 size() const = 0;
	};

	class directoryListVirtual : public directoryListClass
	{
	public:
		const string path;
		directoryListVirtual(const string &path);
		virtual void next() = 0;
		virtual bool valid() const = 0;
		virtual string name() const = 0;
		string fullPath() const;
	};

	class archiveVirtual
	{
	public:
		virtual void createDirectories(const string &path) = 0;
		virtual bool exists(const string &path) = 0;
		virtual bool isDirectory(const string &path) = 0;
		virtual void move(const string &from, const string &to) = 0;
		virtual void remove(const string &path) = 0;
		virtual uint64 lastChange(const string &path) = 0;
		virtual holder<fileClass> file(const string &path, const fileMode &mode) = 0;
		virtual holder<directoryListClass> directoryList(const string &path) = 0;
	};

	void archiveMove(std::shared_ptr<archiveVirtual> &af, const string &pf, std::shared_ptr<archiveVirtual> &at, const string &pt);
	std::shared_ptr<archiveVirtual> archiveTryOpen(const string &path, string &insidePath);

	void realCreateDirectories(const string &path);
	bool realExists(const string &path);
	bool realIsDirectory(const string &path);
	void realMove(const string &from, const string &to);
	void realRemove(const string &path);
	uint64 realLastChange(const string &path);
	holder<fileClass> realNewFile(const string &path, const fileMode &mode);
	holder<directoryListClass> realNewDirectoryList(const string &path);
}
