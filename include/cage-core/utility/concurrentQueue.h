#ifndef guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
#define guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5

namespace cage
{
	struct CAGE_API concurrentQueueCreateConfig
	{
		memoryArena arena;
		uint32 maxElements;
		concurrentQueueCreateConfig();
	};

	namespace privat
	{
		class CAGE_API concurrentQueuePriv
		{
		public:
			void push(void *value);
			bool tryPush(void *value);
			void pop(void *&value);
			bool tryPop(void *&value);
			uint32 estimatedSize() const;
			memoryArena arena;
		};

		CAGE_API holder<concurrentQueuePriv> newConcurrentQueue(const concurrentQueueCreateConfig &config);
	}

	template<class T>
	class concurrentQueueClass
	{
	public:
		concurrentQueueClass(holder<privat::concurrentQueuePriv> queue) : queue(templates::move(queue)) {}

		void push(const T &value) { queue->push(queue->arena.createObject<T>(value)); }
		void push(T &&value) { queue->push(queue->arena.createObject<T>(templates::move(value))); }
		bool tryPush(const T &value) { return queue->tryPush(queue->arena.createObject<T>(value)); }
		bool tryPush(T &&value) { return queue->tryPush(queue->arena.createObject<T>(templates::move(value))); }

		void pop(T &value)
		{
			void *tmp = nullptr;
			queue->pop(tmp);
			CAGE_ASSERT_RUNTIME(tmp);
			try
			{
				value = *(T*)tmp;
			}
			catch(...)
			{
				queue->arena.deallocate(tmp);
				throw;
			}
			queue->arena.deallocate(tmp);
		}

		bool tryPop(T &value)
		{
			void *tmp = nullptr;
			if (queue->tryPop(tmp))
			{
				CAGE_ASSERT_RUNTIME(tmp);
				try
				{
					value = *(T*)tmp;
				}
				catch(...)
				{
					queue->arena.deallocate(tmp);
					throw;
				}
				queue->arena.deallocate(tmp);
				return true;
			}
			return false;
		}

		uint32 estimatedSize() const { return queue->estimatedSize(); }

	private:
		holder<privat::concurrentQueuePriv> queue;
	};

	template<class T>
	class concurrentQueueClass<T*>
	{
	public:
		concurrentQueueClass(holder<privat::concurrentQueuePriv> queue) : queue(templates::move(queue)) {}
		void push(T *value) { queue->push(value); }
		bool tryPush(T *value) { return queue->tryPush(value); }
		void pop(T *&value) { void *tmp; queue->pop(tmp); value = (T*)tmp; }
		bool tryPop(T *&value) { void *tmp; if (queue->tryPop(tmp)) { value = (T*)tmp; return true; } return false; }
		uint32 estimatedSize() const { return queue->estimatedSize(); }

	private:
		holder<privat::concurrentQueuePriv> queue;
	};

	template<class T>
	holder<concurrentQueueClass<T>> newConcurrentQueue(const concurrentQueueCreateConfig &config)
	{
		return detail::systemArena().createHolder<concurrentQueueClass<T>>(privat::newConcurrentQueue(config));
	}
}

#endif // guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
