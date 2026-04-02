#ifndef guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
#define guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5

#include <cage-core/concurrent.h>

namespace cage
{
	struct CAGE_CORE_API ConcurrentQueueTerminated : public Exception
	{
		using Exception::Exception;
	};

	template<class T, template<class...> class Container>
	class ConcurrentQueue : private Immovable
	{
	public:
		explicit ConcurrentQueue(uint32 maxItems = m) : maxItems(maxItems) {}

		~ConcurrentQueue()
		{
			ScopeLock sl(mut); // mandate memory barriers
		}

		void push(const T &value, bool ignoreStop = false)
		{
			ScopeLock sl(mut);
			while (true)
			{
				if (stop && !ignoreStop)
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

		void push(T &&value, bool ignoreStop = false)
		{
			ScopeLock sl(mut);
			while (true)
			{
				if (stop && !ignoreStop)
					CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
				if (items.size() >= maxItems)
					writer->wait(sl);
				else
				{
					items.push_back(std::move(value));
					reader->signal();
					return;
				}
			}
		}

		bool tryPush(const T &value, bool ignoreStop = false)
		{
			ScopeLock sl(mut);
			if (stop && !ignoreStop)
				CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
			if (items.size() < maxItems)
			{
				items.push_back(value);
				reader->signal();
				return true;
			}
			return false;
		}

		bool tryPush(T &&value, bool ignoreStop = false)
		{
			ScopeLock sl(mut);
			if (stop && !ignoreStop)
				CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
			if (items.size() < maxItems)
			{
				items.push_back(std::move(value));
				reader->signal();
				return true;
			}
			return false;
		}

		void pop(T &value, bool ignoreStop = false)
		{
			ScopeLock sl(mut);
			while (true)
			{
				if (stop && !ignoreStop)
					CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
				if (items.empty())
					reader->wait(sl);
				else
				{
					value = std::move(items.front());
					internalPopFront();
					writer->signal();
					return;
				}
			}
		}

		bool tryPop(T &value, bool ignoreStop = false)
		{
			ScopeLock sl(mut);
			if (stop && !ignoreStop)
				CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
			if (!items.empty())
			{
				value = std::move(items.front());
				internalPopFront();
				writer->signal();
				return true;
			}
			return false;
		}

		void terminate()
		{
			ScopeLock sl(mut); // mandate memory barriers
			stop = true;
			// broadcast before unlock
			writer->broadcast();
			reader->broadcast();
		}

		CAGE_FORCE_INLINE bool stopped() const
		{
			ScopeLock sl(mut); // mandate memory barriers
			return stop;
		}

		CAGE_FORCE_INLINE uint32 estimatedSize() const noexcept { return numeric_cast<uint32>(items.size()); }

	protected:
		Holder<Mutex> mut = newMutex();
		Holder<ConditionalVariable> writer = newConditionalVariable();
		Holder<ConditionalVariable> reader = newConditionalVariable();
		Container<T> items;
		uint32 maxItems = m;
		bool stop = false;

		CAGE_FORCE_INLINE void internalPopFront()
		{
			if constexpr (requires(Container<T> c) { c.pop_front(); })
				items.pop_front();
			else
				items.erase(items.begin());
		}
	};
}

#endif // guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
