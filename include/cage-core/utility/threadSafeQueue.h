#ifndef guard_threadSafeQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
#define guard_threadSafeQueue_h_F17509C840DB4228AF89C97FCD8EC1E5

namespace cage
{
	struct CAGE_API threadSafeQueueCreateConfig
	{
		memoryArena arena;
		uint32 maxElements;
		threadSafeQueueCreateConfig();
	};

	namespace privat
	{
		class CAGE_API threadSafeQueuePriv
		{
		public:
			void push(void *value);
			bool tryPush(void *value);
			void pop(void *&value);
			bool tryPop(void *&value);
			memoryArena arena;
		};

		CAGE_API holder<threadSafeQueuePriv> newThreadSafeQueue(const threadSafeQueueCreateConfig &config);
	}

	template<class T>
	class threadSafeQueueClass
	{
	public:
		threadSafeQueueClass(holder<privat::threadSafeQueuePriv> queue) : queue(templates::move(queue)) {}

		void push(const T &value) { queue->push(queue->arena.createObject<T>(value)); }
		void push(T &&value) { queue->push(queue->arena.createObject<T>(templates::move(value))); }
		bool tryPush(const T &value) { return queue->tryPush(queue->arena.createObject<T>(value)); }
		bool tryPush(T &&value) { return queue->tryPush(queue->arena.createObject<T>(templates::move(value))); }

		void pop(T &value)
		{
			void *tmp = nullptr;
			queue->pop(tmp);
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

	private:
		holder<privat::threadSafeQueuePriv> queue;
	};

	template<class T>
	class threadSafeQueueClass<T*>
	{
	public:
		threadSafeQueueClass(holder<privat::threadSafeQueuePriv> queue) : queue(templates::move(queue)) {}
		void push(T *value) { queue->push(value); }
		bool tryPush(T *value) { return queue->tryPush(value); }
		void pop(T *&value) { void *tmp; queue->pop(tmp); value = (T*)tmp; }
		bool tryPop(T *&value) { void *tmp; if (queue->tryPop(tmp)) { value = (T*)tmp; return true; } return false; }

	private:
		holder<privat::threadSafeQueuePriv> queue;
	};

	template<class T>
	holder<threadSafeQueueClass<T>> newThreadSafeQueue(const threadSafeQueueCreateConfig &config)
	{
		return detail::systemArena().createHolder<threadSafeQueueClass<T>>(privat::newThreadSafeQueue(config));
	}
}

#endif // guard_threadSafeQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
