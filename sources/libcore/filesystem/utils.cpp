#include <set>
#include <FileWatcher/FileWatcher.h>

#include "filesystem.h"
#include <cage-core/timer.h>
#include <cage-core/concurrent.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <dirent.h>
#endif

namespace cage
{
	namespace
	{
		class changeWatcherImpl : public filesystemWatcher, public FW::FileWatchListener
		{
		public:
			std::set<string, stringComparatorFast> files;
			holder<FW::FileWatcher> fw;
			holder<timer> timer;

			changeWatcherImpl()
			{
				fw = detail::systemArena().createHolder<FW::FileWatcher>();
				timer = newTimer();
			}

			const string waitForChange(uint64 time)
			{
				timer->reset();
				while (files.empty())
				{
					fw->update();
					if (files.empty() && timer->microsSinceStart() > time)
						return "";
					else
						threadSleep(1000 * 100);
				}
				string res = *files.begin();
				files.erase(files.begin());
				return res;
			}

			virtual void handleFileAction(FW::WatchID watchid, const FW::String &dir, const FW::String &filename, FW::Action action)
			{
				files.insert(pathJoin(dir.c_str(), filename.c_str()));
			}
		};
	}

	void filesystemWatcher::registerPath(const string & path)
	{
		changeWatcherImpl *impl = (changeWatcherImpl*)this;
		impl->fw->addWatch(path.c_str(), impl);
		holder<directoryList> dl = newDirectoryList(path);
		while (dl->valid())
		{
			if (dl->isDirectory())
				registerPath(pathJoin(path, dl->name()));
			dl->next();
		}
	}

	string filesystemWatcher::waitForChange(uint64 time)
	{
		changeWatcherImpl *impl = (changeWatcherImpl*)this;
		return impl->waitForChange(time);
	}

	holder<filesystemWatcher> newFilesystemWatcher()
	{
		return detail::systemArena().createImpl<filesystemWatcher, changeWatcherImpl>();
	}

	directoryListVirtual::directoryListVirtual(const string &path) : myPath(path)
	{}

	bool directoryList::valid() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return impl->valid();
	}

	string directoryList::name() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return impl->name();
	}

	pathTypeFlags directoryList::type() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return pathType(impl->fullPath());
	}

	bool directoryList::isDirectory() const
	{
		return any(type() & (pathTypeFlags::Directory | pathTypeFlags::Archive));
	}

	uint64 directoryList::lastChange() const
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return pathLastChange(impl->fullPath());
	}

	holder<fileHandle> directoryList::openFile(const fileMode &mode)
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		return newFile(impl->fullPath(), mode);
	}

	holder<filesystem> directoryList::openDirectory()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(exception, "is not dir");
		holder<filesystem> fs = newFilesystem();
		fs->changeDir(impl->fullPath());
		return fs;
	}

	holder<directoryList> directoryList::listDirectory()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(exception, "is not dir");
		return newDirectoryList(impl->fullPath());
	}

	void directoryList::next()
	{
		directoryListVirtual *impl = (directoryListVirtual *)this;
		impl->next();
	}

	string directoryListVirtual::fullPath() const
	{
		return pathJoin(myPath, name());
	}

	holder<directoryList> newDirectoryList(const string &path)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, true, p);
		if (a)
			return a->listDirectory(p);
		else
			return realNewDirectoryList(path);
	}

	namespace
	{
		class directoryListReal : public directoryListVirtual
		{
		public:
			bool valid_;

#ifdef CAGE_SYSTEM_WINDOWS
			WIN32_FIND_DATA ffd;
			HANDLE list;
#else
			DIR *pdir;
			struct dirent *pent;
#endif

			directoryListReal(const string &path) : directoryListVirtual(path), valid_(false)
#ifdef CAGE_SYSTEM_WINDOWS
				, list(nullptr)
#else
				, pdir(nullptr), pent(nullptr)
#endif
			{
				realCreateDirectories(path);

#ifdef CAGE_SYSTEM_WINDOWS

				/*
				HANDLE hFile = CreateFile(path.c_str(), 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					char buffer[string::MaxLength];
					DWORD len = GetFinalPathNameByHandle(hFile, buffer, string::MaxLength, VOLUME_NAME_DOS);
					CloseHandle(hFile);
					if (len < string::MaxLength)
						const_cast<string&>(myPath) = string(buffer + 4);
				}
				*/

				list = FindFirstFile(pathJoin(myPath, "*").c_str(), &ffd);
				valid_ = list != INVALID_HANDLE_VALUE;
				if (!valid_)
					return;
				if (name() == "." || name() == "..")
					next();

#else

				pdir = opendir(path.c_str());
				valid_ = !!pdir;
				next();

#endif
			}

			~directoryListReal()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				if (list)
					FindClose(list);
#else
				if (pdir)
					closedir(pdir);
#endif
			}

			bool valid() const override
			{
				return valid_;
			}

			string name() const override
			{
#ifdef CAGE_SYSTEM_WINDOWS
				return ffd.cFileName;
#else
				return pent->d_name;
#endif
			}

			void next() override
			{
				CAGE_ASSERT_RUNTIME(valid_, "can not advance on invalid list");

#ifdef CAGE_SYSTEM_WINDOWS

				if (FindNextFile(list, &ffd) == 0)
				{
					valid_ = false;
					return;
				}

#else

				pent = readdir(pdir);
				if (!pent)
				{
					valid_ = false;
					return;
				}

#endif

				if (name() == "." || name() == "..")
					next();
			}
		};
	}

	holder<directoryList> realNewDirectoryList(const string &path)
	{
		return detail::systemArena().createImpl<directoryList, directoryListReal>(path);
	}

	namespace
	{
		class filesystemImpl : public filesystem
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

	void filesystem::changeDir(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		impl->current = impl->makePath(path);
	}

	string filesystem::currentDir() const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return impl->current;
	}

	pathTypeFlags filesystem::type(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathType(impl->makePath(path));
	}

	uint64 filesystem::lastChange(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathLastChange(impl->makePath(path));
	}

	holder<fileHandle> filesystem::openFile(const string &path, const fileMode &mode)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return newFile(impl->makePath(path), mode);
	}

	holder<directoryList> filesystem::listDirectory(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return newDirectoryList(impl->makePath(path));
	}

	holder<filesystemWatcher> filesystem::watchFilesystem(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		holder<filesystemWatcher> cw = newFilesystemWatcher();
		cw->registerPath(impl->makePath(path));
		return cw;
	}

	void filesystem::move(const string &from, const string &to)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathMove(impl->makePath(from), impl->makePath(to));
	}

	void filesystem::remove(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathRemove(impl->makePath(path));
	}

	holder<filesystem> newFilesystem()
	{
		return detail::systemArena().createImpl<filesystem, filesystemImpl>();
	}
}
