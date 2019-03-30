#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/filesystem.h>

namespace cage
{
	namespace
	{
		class filesystemImpl : public filesystemClass
		{
		public:
			string current;

			string makePath(const string &path) const
			{
				if (pathIsAbs(path))
					return pathToRel(path);
				else
					return pathToRel(pathJoin(current, path));
			}
		};
	}

	void filesystemClass::changeDir(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		impl->current = impl->makePath(path);
	}

	string filesystemClass::currentDir() const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return impl->current;
	}

	pathTypeFlags filesystemClass::type(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathType(impl->makePath(path));
	}

	uint64 filesystemClass::lastChange(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathLastChange(impl->makePath(path));
	}

	holder<fileClass> filesystemClass::openFile(const string &path, const fileMode &mode)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return newFile(impl->makePath(path), mode);
	}

	holder<directoryListClass> filesystemClass::directoryList(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return newDirectoryList(impl->makePath(path));
	}

	holder<changeWatcherClass> filesystemClass::changeWatcher(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		holder<changeWatcherClass> cw = newChangeWatcher();
		cw->registerPath(impl->makePath(path));
		return cw;
	}

	void filesystemClass::move(const string &from, const string &to)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathMove(impl->makePath(from), impl->makePath(to));
	}

	void filesystemClass::remove(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathRemove(impl->makePath(path));
	}

	holder<filesystemClass> newFilesystem()
	{
		return detail::systemArena().createImpl<filesystemClass, filesystemImpl>();
	}
}
