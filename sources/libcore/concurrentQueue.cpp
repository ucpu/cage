#include <list>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>

namespace cage
{
	ConcurrentQueueCreateConfig::ConcurrentQueueCreateConfig() : arena(detail::systemArena()), maxElements(m) {}

	ConcurrentQueueTerminated::ConcurrentQueueTerminated(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept : Exception(file, line, function, severity, message)
	{};

	namespace
	{
		class concurrentQueueImpl : public privat::ConcurrentQueuePriv
		{
		public:
			concurrentQueueImpl(const ConcurrentQueueCreateConfig &config) : maxItems(config.maxElements), stop(false)
			{
				mut = newSyncMutex();
				writer = newSyncConditionalBase();
				reader = newSyncConditionalBase();
				arena = config.arena;
			}

			~concurrentQueueImpl()
			{
				ScopeLock<Mutex> sl(mut);
				CAGE_ASSERT(items.empty(), "the concurrent queue may not be destroyed before all items are removed");
			}

			void push(void *value)
			{
				ScopeLock<Mutex> sl(mut);
				while (true)
				{
					if (stop)
						CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
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
				ScopeLock<Mutex> sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
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
				ScopeLock<Mutex> sl(mut);
				while (true)
				{
					if (stop)
						CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
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
				ScopeLock<Mutex> sl(mut);
				if (stop)
					CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
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
				ScopeLock<Mutex> sl(mut);
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
					ScopeLock<Mutex> sl(mut);
					stop = true;
				}
				writer->broadcast();
				reader->broadcast();
			}

			bool stopped() const
			{
				ScopeLock<Mutex> sl(mut); // mandate memory barriers
				return stop;
			}

			mutable Holder<Mutex> mut;
			Holder<ConditionalVariableBase> writer, reader;
			std::list<void*> items;
			uint32 maxItems;
			bool stop;
		};
	}

	namespace privat
	{
		void ConcurrentQueuePriv::push(void *value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->push(value);
		}

		bool ConcurrentQueuePriv::tryPush(void *value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->tryPush(value);
		}

		void ConcurrentQueuePriv::pop(void *&value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->pop(value);
		}

		bool ConcurrentQueuePriv::tryPop(void *&value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->tryPop(value);
		}

		bool ConcurrentQueuePriv::tryPopNoStop(void *&value)
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->tryPopNoStop(value);
		}

		uint32 ConcurrentQueuePriv::estimatedSize() const
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return numeric_cast<uint32>(impl->items.size());
		}

		void ConcurrentQueuePriv::terminate()
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			impl->terminate();
		}

		bool ConcurrentQueuePriv::stopped() const
		{
			concurrentQueueImpl *impl = (concurrentQueueImpl *)this;
			return impl->stopped();
		}

		Holder<ConcurrentQueuePriv> newConcurrentQueue(const ConcurrentQueueCreateConfig &config)
		{
			return detail::systemArena().createImpl<ConcurrentQueuePriv, concurrentQueueImpl>(config);
		}
	}
}

