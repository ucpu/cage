#include <deque>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadSafeQueue.h>

namespace cage
{
	threadSafeQueueCreateConfig::threadSafeQueueCreateConfig() : arena(detail::systemArena()), maxElements(1000) {}

	namespace
	{
		class threadSafeQueueImpl : public privat::threadSafeQueuePriv
		{
		public:
			threadSafeQueueImpl(const threadSafeQueueCreateConfig &config) : maxItems(config.maxElements)
			{
				mutex = newMutex();
				writer = newConditionalBase();
				reader = newConditionalBase();
				arena = config.arena;
			}

			~threadSafeQueueImpl()
			{
				scopeLock<mutexClass> sl(mutex);
				CAGE_ASSERT_RUNTIME(items.empty(), "the thread safe queue may not be destroyed before all items are removed");
			}

			void push(void *value)
			{
				CAGE_ASSERT_RUNTIME(value);
				scopeLock<mutexClass> sl(mutex);
				while (true)
				{
					if (items.size() >= maxItems)
						writer->wait(sl);
					else
					{
						items.push_back(value);
						reader->signal();
						return;
					}
				}
			}

			bool tryPush(void *value)
			{
				CAGE_ASSERT_RUNTIME(value);
				scopeLock<mutexClass> sl(mutex);
				if (items.size() < maxItems)
				{
					items.push_back(value);
					reader->signal();
					return true;
				}
				return false;
			}

			void pop(void *&value)
			{
				scopeLock<mutexClass> sl(mutex);
				while (true)
				{
					if (items.empty())
						reader->wait(sl);
					else
					{
						value = items.front();
						items.pop_front();
						writer->signal();
						return;
					}
				}
			}

			bool tryPop(void *&value)
			{
				scopeLock<mutexClass> sl(mutex);
				if (!items.empty())
				{
					value = items.front();
					items.pop_front();
					writer->signal();
					return true;
				}
				return false;
			}

			holder<mutexClass> mutex;
			holder<conditionalBaseClass> writer, reader;
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

		void threadSafeQueuePriv::pop(void *&value)
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

