#include <deque>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>

namespace cage
{
	concurrentQueueCreateConfig::concurrentQueueCreateConfig() : arena(detail::systemArena()), maxElements(1000) {}

	namespace
	{
		class concurrentQueueImpl : public privat::concurrentQueuePriv
		{
		public:
			concurrentQueueImpl(const concurrentQueueCreateConfig &config) : maxItems(config.maxElements)
			{
				mutex = newMutex();
				writer = newConditionalBase();
				reader = newConditionalBase();
				arena = config.arena;
			}

			~concurrentQueueImpl()
			{
				scopeLock<mutexClass> sl(mutex);
				CAGE_ASSERT_RUNTIME(items.empty(), "the concurrent queue may not be destroyed before all items are removed");
			}

			void push(void *value)
			{
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
		void concurrentQueuePriv::push(void *value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->push(value);
		}

		bool concurrentQueuePriv::tryPush(void *value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->tryPush(value);
		}

		void concurrentQueuePriv::pop(void *&value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->pop(value);
		}

		bool concurrentQueuePriv::tryPop(void *&value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->tryPop(value);
		}

		uint32 concurrentQueuePriv::estimatedSize() const
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return numeric_cast<uint32>(impl->items.size());
		}

		holder<concurrentQueuePriv> newConcurrentQueue(const concurrentQueueCreateConfig &config)
		{
			return detail::systemArena().createImpl<concurrentQueuePriv, concurrentQueueImpl>(config);
		}
	}
}

