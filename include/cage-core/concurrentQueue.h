#ifndef guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
#define guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5

#include <cage-core/concurrent.h>

#include <list>

namespace cage
{
	struct CAGE_API ConcurrentQueueTerminated : public Exception
	{
		ConcurrentQueueTerminated(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept : Exception(file, line, function, severity, message)
		{};
	};

	template<class T>
	class ConcurrentQueue : private Immovable
	{
	public:
		explicit ConcurrentQueue(uint32 maxItems = m) : maxItems(maxItems), stop(false)
		{
			mut = newMutex();
			writer = newConditionalVariableBase();
			reader = newConditionalVariableBase();
		}

		void push(const T &value)
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

		void push(T &&value)
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
					items.push_back(templates::move(value));
					reader->signal();
					return;
				}
			}
		}

		bool tryPush(const T &value)
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

		bool tryPush(T &&value)
		{
			ScopeLock<Mutex> sl(mut);
			if (stop)
				CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
			if (items.size() < maxItems)
			{
				items.push_back(templates::move(value));
				reader->signal();
				return true;
			}
			return false;
		}

		void pop(T &value)
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
					value = templates::move(items.front());
					items.pop_front();
					writer->signal();
					return;
				}
			}
		}

		bool tryPop(T &value)
		{
			ScopeLock<Mutex> sl(mut);
			if (stop)
				CAGE_THROW_SILENT(ConcurrentQueueTerminated, "concurrent queue terminated");
			if (!items.empty())
			{
				value = templates::move(items.front());
				items.pop_front();
				writer->signal();
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

		uint32 estimatedSize() const
		{
			return numeric_cast<uint32>(items.size());
		}

	private:
		Holder<Mutex> mut;
		Holder<ConditionalVariableBase> writer, reader;
		std::list<T> items;
		uint32 maxItems;
		bool stop;
	};
}

#endif // guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
