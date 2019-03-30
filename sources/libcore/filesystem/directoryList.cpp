#include "filesystem.h"

namespace cage
{
	directoryListVirtual::directoryListVirtual(const string &path) : myPath(path)
	{}

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

	pathTypeFlags directoryListClass::type() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return pathType(impl->fullPath());
	}

	bool directoryListClass::isDirectory() const
	{
		return (type() & (pathTypeFlags::Directory | pathTypeFlags::Archive)) != pathTypeFlags::None;
	}

	uint64 directoryListClass::lastChange() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return pathLastChange(impl->fullPath());
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

	void directoryListClass::next()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		impl->next();
	}

	string directoryListVirtual::fullPath() const
	{
		return pathJoin(myPath, name());
	}

	holder<directoryListClass> newDirectoryList(const string &path)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, true, p);
		if (a)
			return a->directoryList(p);
		else
			return realNewDirectoryList(path);
	}
}
