#include "filesystem.h"

namespace cage
{
	directoryListVirtual::directoryListVirtual(const string &path) : path(path)
	{}

	string directoryListVirtual::fullPath() const
	{
		return pathJoin(path, name());
	}

	void directoryListClass::next()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		impl->next();
	}

	holder<fileClass> directoryListClass::openFile(const fileMode &mode)
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return newFile(impl->fullPath(), mode);
	}

	holder<filesystemClass> directoryListClass::openDirectory()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(exception, "is not dir");
		holder<filesystemClass> fs = newFilesystem();
		fs->changeDir(impl->fullPath());
		return fs;
	}

	holder<directoryListClass> directoryListClass::directoryList()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(exception, "is not dir");
		return newDirectoryList(impl->fullPath());
	}

	bool directoryListClass::valid() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return impl->valid();
	}

	string directoryListClass::name() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return impl->name();
	}

	bool directoryListClass::isDirectory() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return pathIsDirectory(impl->fullPath());
	}

	uint64 directoryListClass::lastChange() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return pathLastChange(impl->fullPath());
	}

	holder<directoryListClass> newDirectoryList(const string &path)
	{
		string p;
		auto a = archiveTryOpen(path, p);
		if (a)
			return a->directoryList(p);
		else
			return realNewDirectoryList(path);
	}
}
