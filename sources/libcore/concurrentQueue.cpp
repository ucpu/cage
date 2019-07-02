#include <list>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>

namespace cage
{
	concurrentQueueCreateConfig::concurrentQueueCreateConfig() : arena(detail::systemArena()), maxElements(m) {}

	concurrentQueueTerminated::concurrentQueueTerminated(GCHL_EXCEPTION_GENERATE_CTOR_PARAMS) noexcept : exception(GCHL_EXCEPTION_GENERATE_CTOR_INITIALIZER)
	{};

	namespace
	{
		class concurrentQueueImpl : public privat::concurrentQueuePriv
		{
		public:
			concurrentQueueImpl(const concurrentQueueCreateConfig &config) : maxItems(config.maxElements), stop(false)
			{
				mut = newSyncMutex();
				writer = newSyncConditionalBase();
				reader = newSyncConditionalBase();
				arena = config.arena;
			}

			~concurrentQueueImpl()
			{
				scopeLock<syncMutex> sl(mut);
				CAGE_ASSERT_RUNTIME(items.empty(), "the concurrent queue may not be destroyed before all items are removed");
			}

			void push(void *value)
			{
				scopeLock<syncMutex> sl(mut);
				while (true)
				{
					if (stop)
						CAGE_THROW_SILENT(concurrentQueueTerminated, "concurrent queue terminated");
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
				scopeLock<syncMutex> sl(mut);
				if (stop)
					CAGE_THROW_SILENT(concurrentQueueTerminated, "concurrent queue terminated");
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
				scopeLock<syncMutex> sl(mut);
				while (true)
				{
					if (stop)
						CAGE_THROW_SILENT(concurrentQueueTerminated, "concurrent queue terminated");
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
				scopeLock<syncMutex> sl(mut);
				if (stop)
					CAGE_THROW_SILENT(concurrentQueueTerminated, "concurrent queue terminated");
				if (!items.empty())
				{
					value = items.front();
					items.pop_front();
					writer->signal();
					return true;
				}
				return false;
			}

			bool tryPopNoStop(void *&value)
			{
				scopeLock<syncMutex> sl(mut);
				if (!items.empty())
				{
					value = items.front();
					items.pop_front();
					return true;
				}
				return false;
			}

			void terminate()
			{
				{
					scopeLock<syncMutex> sl(mut);
					stop = true;
				}
				writer->broadcast();
				reader->broadcast();
			}

			bool stopped() const
			{
				scopeLock<syncMutex> sl(mut); // mandate memory barriers
				return stop;
			}

			mutable holder<syncMutex> mut;
			holder<syncConditionalBase> writer, reader;
			std::list<void*> items;
			uint32 maxItems;
			bool stop;
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

		bool concurrentQueuePriv::tryPopNoStop(void *&value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->tryPopNoStop(value);
		}

		uint32 concurrentQueuePriv::estimatedSize() const
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return numeric_cast<uint32>(impl->items.size());
		}

		void concurrentQueuePriv::terminate()
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			impl->terminate();
		}

		bool concurrentQueuePriv::stopped() const
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->stopped();
		}

		holder<concurrentQueuePriv> newConcurrentQueue(const concurrentQueueCreateConfig &config)
		{
			return detail::systemArena().createImpl<concurrentQueuePriv, concurrentQueueImpl>(config);
		}
	}
}

