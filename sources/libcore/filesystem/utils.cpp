#include "filesystem.h"
#include <cage-core/timer.h>
#include <cage-core/concurrent.h>
#include <cage-core/string.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <dirent.h>
#endif

#include <FileWatcher/FileWatcher.h>

#include <set>

namespace cage
{
	namespace
	{
		class FilesystemWatcherImpl : public FilesystemWatcher, public FW::FileWatchListener
		{
		public:
			std::set<string, stringComparatorFast> files;
			Holder<FW::FileWatcher> fw;
			Holder<Timer> clock;

			FilesystemWatcherImpl()
			{
				fw = detail::systemArena().createHolder<FW::FileWatcher>();
				clock = newTimer();
			}

			const string waitForChange(uint64 time)
			{
				clock->reset();
				while (files.empty())
				{
					fw->update();
					if (files.empty() && clock->microsSinceStart() > time)
						return "";
					else
						threadSleep(1000 * 100);
				}
				string res = *files.begin();
				files.erase(files.begin());
				return res;
			}

			virtual void handleFileAction(FW::WatchID watchid, const FW::String &dir, const FW::String &filename, FW::Action action) override
			{
				files.insert(pathJoin(dir.c_str(), filename.c_str()));
			}
		};
	}

	void FilesystemWatcher::registerPath(const string & path)
	{
		FilesystemWatcherImpl *impl = (FilesystemWatcherImpl*)this;
		impl->fw->addWatch(path.c_str(), impl);
		Holder<DirectoryList> dl = newDirectoryList(path);
		while (dl->valid())
		{
			auto type = dl->type();
			if (any(type & PathTypeFlags::Directory) && none(type & (PathTypeFlags::Archive | PathTypeFlags::InsideArchive)))
				registerPath(pathJoin(path, dl->name()));
			dl->next();
		}
	}

	string FilesystemWatcher::waitForChange(uint64 time)
	{
		FilesystemWatcherImpl *impl = (FilesystemWatcherImpl*)this;
		return impl->waitForChange(time);
	}

	Holder<FilesystemWatcher> newFilesystemWatcher()
	{
		return detail::systemArena().createImpl<FilesystemWatcher, FilesystemWatcherImpl>();
	}

	DirectoryListAbstract::DirectoryListAbstract(const string &path) : myPath(path)
	{}

	bool DirectoryList::valid() const
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		return impl->valid();
	}

	string DirectoryList::name() const
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		return impl->name();
	}

	PathTypeFlags DirectoryList::type() const
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		return pathType(impl->fullPath());
	}

	bool DirectoryList::isDirectory() const
	{
		return any(type() & (PathTypeFlags::Directory | PathTypeFlags::Archive));
	}

	uint64 DirectoryList::lastChange() const
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		return pathLastChange(impl->fullPath());
	}

	Holder<File> DirectoryList::openFile(const FileMode &mode)
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		return newFile(impl->fullPath(), mode);
	}

	Holder<File> DirectoryList::readFile()
	{
		return openFile(FileMode(true, false));
	}

	Holder<File> DirectoryList::writeFile()
	{
		return openFile(FileMode(false, true));
	}

	Holder<DirectoryList> DirectoryList::listDirectory()
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(Exception, "is not dir");
		return newDirectoryList(impl->fullPath());
	}

	void DirectoryList::next()
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		impl->next();
	}

	string DirectoryListAbstract::fullPath() const
	{
		return pathJoin(myPath, name());
	}

	Holder<DirectoryList> newDirectoryList(const string &path)
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
		class DirectoryListReal : public DirectoryListAbstract
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

			DirectoryListReal(const string &path) : DirectoryListAbstract(path), valid_(false)
#ifdef CAGE_SYSTEM_WINDOWS
				, list(nullptr)
#else
				, pdir(nullptr), pent(nullptr)
#endif
			{
				realCreateDirectories(path);

#ifdef CAGE_SYSTEM_WINDOWS

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

			~DirectoryListReal()
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
				CAGE_ASSERT(valid_);

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

	Holder<DirectoryList> realNewDirectoryList(const string &path)
	{
		return detail::systemArena().createImpl<DirectoryList, DirectoryListReal>(path);
	}
}
