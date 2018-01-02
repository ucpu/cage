#include <deque>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadSafeQueue.h>

namespace cage
{
	threadSafeQueueCreateConfig::threadSafeQueueCreateConfig() : maxElements(1000) {}

	namespace
	{
		class threadSafeQueueImpl : public privat::threadSafeQueuePriv
		{
		public:
			threadSafeQueueImpl(const threadSafeQueueCreateConfig &config) : maxItems(config.maxElements)
			{
				mutex = newMutex();
				//writers = newMutex();
				//readers = newMutex();
				arena = detail::systemArena();
			}

			~threadSafeQueueImpl()
			{
				scopeLock<mutexClass> sl(mutex);
				CAGE_ASSERT_RUNTIME(items.empty(), "the thread safe queue may not be destroyed before all items are removed");
			}

			void push(void *value)
			{
				if (tryPush(value))
					return;
				//writers->lock();
				threadSleep(1);
				return push(value);
			}

			bool tryPush(void *value)
			{
				CAGE_ASSERT_RUNTIME(value);
				scopeLock<mutexClass> sl(mutex);
				if (items.size() < maxItems)
				{
					items.push_back(value);
					//readers->unlock();
					return true;
				}
				return false;
			}

			bool pop(void *&value)
			{
				if (tryPop(value))
					return true;
				//readers->lock();
				threadSleep(1);
				return pop(value);
			}

			bool tryPop(void *&value)
			{
				scopeLock<mutexClass> sl(mutex);
				if (!items.empty())
				{
					value = items.front();
					items.pop_front();
					//writers->unlock();
					return true;
				}
				return false;
			}

			holder<mutexClass> mutex;
			// conditional variables writers, readers;
			std::deque<void*> items;
			uint32 maxItems;
		};
	}

	namespace privat
	{
		void threadSafeQueuePriv::push(void *value)
		{
			threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
			return impl->push(value);
		}

		bool threadSafeQueuePriv::tryPush(void *value)
		{
			threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
			return impl->tryPush(value);
		}

		bool threadSafeQueuePriv::pop(void *&value)
		{
			threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
			return impl->pop(value);
		}

		bool threadSafeQueuePriv::tryPop(void *&value)
		{
			threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
			return impl->tryPop(value);
		}

		holder<threadSafeQueuePriv> newThreadSafeQueue(const threadSafeQueueCreateConfig &config)
		{
			return detail::systemArena().createImpl<threadSafeQueuePriv, threadSafeQueueImpl>(config);
		}
	}
}

