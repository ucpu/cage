#include <set>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/filesystem.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/timer.h>

#include <FileWatcher/FileWatcher.h>

namespace cage
{
	namespace
	{
		class changeWatcherImpl : public changeWatcherClass, public FW::FileWatchListener
		{
		public:
			std::set<string, stringComparatorFast> files;
			holder<FW::FileWatcher> fw;
			holder<timerClass> timer;

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

	void changeWatcherClass::registerPath(const string & path)
	{
		changeWatcherImpl *impl = (changeWatcherImpl*)this;
		impl->fw->addWatch(path.c_str(), impl);
		holder<directoryListClass> dl = newDirectoryList(path);
		while (dl->valid())
		{
			if (dl->isDirectory())
				registerPath(pathJoin(path, dl->name()));
			dl->next();
		}
	}

	string changeWatcherClass::waitForChange(uint64 time)
	{
		changeWatcherImpl *impl = (changeWatcherImpl*)this;
		return impl->waitForChange(time);
	}

	holder<changeWatcherClass> newChangeWatcher()
	{
		return detail::systemArena().createImpl <changeWatcherClass, changeWatcherImpl>();
	}
}