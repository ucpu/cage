#include <FileWatcher/FileWatcher.h>

#include <cage-core/concurrent.h> // threadSleep
#include <cage-core/files.h>
#include <cage-core/flatSet.h>
#include <cage-core/string.h>
#include <cage-core/timer.h>

namespace cage
{
	PathTypeFlags realType(const String &path);

	namespace
	{
		class FilesystemWatcherImpl : public FilesystemWatcher, private FW::FileWatchListener
		{
		public:
			FlatSet<String, StringComparatorFast> files;
			Holder<FW::FileWatcher> fw;
			Holder<Timer> clock;

			FilesystemWatcherImpl()
			{
				fw = systemMemory().createHolder<FW::FileWatcher>();
				clock = newTimer();
			}

			String waitForChange(uint64 time)
			{
				clock->reset();
				while (files.empty())
				{
					fw->update();
					if (files.empty() && clock->duration() > time)
						return "";
					else
						threadSleep(1000 * 100);
				}
				const String res = *files.begin();
				files.erase(files.begin());
				return res;
			}

			void handleFileAction(FW::WatchID watchid, const FW::String &dir, const FW::String &filename, FW::Action action) override { files.insert(pathJoin(dir.c_str(), filename.c_str())); }

			void registerPath(const String &path)
			{
				fw->addWatch(path.c_str(), this);
				auto names = pathListDirectory(path);
				for (const String &p : names)
				{
					const PathTypeFlags type = realType(p);
					if (any(type & PathTypeFlags::Directory))
						registerPath(p);
				}
			}
		};
	}

	void FilesystemWatcher::registerPath(const String &path_)
	{
		FilesystemWatcherImpl *impl = (FilesystemWatcherImpl *)this;
		const String path = pathToAbs(path_);
		const PathTypeFlags type = realType(path); // FilesystemWatcher works with real filesystem only!
		if (none(type & PathTypeFlags::Directory))
		{
			CAGE_LOG_THROW(Stringizer() + "path: " + path);
			CAGE_THROW_ERROR(Exception, "path must be existing folder");
		}
		impl->registerPath(path);
	}

	String FilesystemWatcher::waitForChange(uint64 time)
	{
		FilesystemWatcherImpl *impl = (FilesystemWatcherImpl *)this;
		return impl->waitForChange(time);
	}

	Holder<FilesystemWatcher> newFilesystemWatcher()
	{
		return systemMemory().createImpl<FilesystemWatcher, FilesystemWatcherImpl>();
	}
}
