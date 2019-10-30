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

	struct CAGE_API concurrentQueueTerminated : public exception
	{
		concurrentQueueTerminated(const char *file, uint32 line, const char *function, severityEnum severity, const char *message) noexcept;
	};

	namespace privat
	{
		class CAGE_API concurrentQueuePriv : private immovable
		{
		public:
			void push(void *value);
			bool tryPush(void *value);
			void pop(void *&value);
			bool tryPop(void *&value);
			bool tryPopNoStop(void *&value);
			uint32 estimatedSize() const;
			void terminate();
			bool stopped() const;
			memoryArena arena;
		};

		CAGE_API holder<concurrentQueuePriv> newConcurrentQueue(const concurrentQueueCreateConfig &config);
	}

	template<class T>
	class concurrentQueue : private immovable
	{
	public:
		concurrentQueue(const concurrentQueueCreateConfig &config) : queue(privat::newConcurrentQueue(config))
		{}

		~concurrentQueue()
		{
			void *tmp = nullptr;
			while (queue->tryPopNoStop(tmp))
				queue->arena.deallocate(tmp);
		}

		void push(const T &value)
		{
			T *tmp = queue->arena.createObject<T>(value);
			try
			{
				queue->push(tmp);
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
		}

		void push(T &&value)
		{
			T *tmp = queue->arena.createObject<T>(templates::move(value));
			try
			{
				queue->push(tmp);
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
		}

		bool tryPush(const T &value)
		{
			T *tmp = queue->arena.createObject<T>(value);
			try
			{
				return queue->tryPush(tmp);
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
		}

		bool tryPush(T &&value)
		{
			T *tmp = queue->arena.createObject<T>(templates::move(value));
			try
			{
				return queue->tryPush(tmp);
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
		}

		void pop(T &value)
		{
			void *tmp = nullptr;
			queue->pop(tmp);
			CAGE_ASSERT(tmp);
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
				CAGE_ASSERT(tmp);
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
		void terminate() { queue->terminate(); }
		bool stopped() const { return queue->stopped(); };

	private:
		holder<privat::concurrentQueuePriv> queue;
	};

	template<class T>
	class concurrentQueue<T*> : private immovable
	{
	public:
		concurrentQueue(const concurrentQueueCreateConfig &config, delegate<void(T*)> deleter) : queue(privat::newConcurrentQueue(config)), deleter(deleter)
		{}

		~concurrentQueue()
		{
			void *tmp = nullptr;
			while (queue->tryPopNoStop(tmp))
			{
				if (deleter)
					deleter((T*)tmp);
			}
		}

		void push(T *value) { queue->push(value); }
		bool tryPush(T *value) { return queue->tryPush(value); }
		void pop(T *&value) { void *tmp; queue->pop(tmp); value = (T*)tmp; }
		bool tryPop(T *&value) { void *tmp; if (queue->tryPop(tmp)) { value = (T*)tmp; return true; } return false; }
		uint32 estimatedSize() const { return queue->estimatedSize(); }
		void terminate() { queue->terminate(); }
		bool stopped() const { return queue->stopped(); };

	private:
		holder<privat::concurrentQueuePriv> queue;
		delegate<void(T*)> deleter;
	};

	template<class T>
	holder<concurrentQueue<T>> newConcurrentQueue(const concurrentQueueCreateConfig &config)
	{
		return detail::systemArena().createHolder<concurrentQueue<T>>(config);
	}

	template<class T>
	holder<concurrentQueue<T>> newConcurrentQueue(const concurrentQueueCreateConfig &config, delegate<void(T)> deleter)
	{
		return detail::systemArena().createHolder<concurrentQueue<T>>(config, deleter);
	}
}

#endif // guard_concurrentQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
