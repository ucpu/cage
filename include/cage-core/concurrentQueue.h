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
			T *tmp = nullptr;
			while (queue->tryPopNoStop((void*&)tmp))
				queue->arena.destroy<T>(tmp);
		}

		void push(const T &value)
		{
			T *tmp = nullptr;
			try
			{
				tmp = queue->arena.createObject<T>(value);
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
			T *tmp = nullptr;
			try
			{
				tmp = queue->arena.createObject<T>(templates::move(value));
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
			T *tmp = nullptr;
			try
			{
				tmp = queue->arena.createObject<T>(value);
				bool ret = queue->tryPush(tmp);
				if (!ret)
					queue->arena.destroy<T>(tmp);
				return ret;
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
		}

		bool tryPush(T &&value)
		{
			T *tmp = nullptr;
			try
			{
				tmp = queue->arena.createObject<T>(templates::move(value));
				bool ret = queue->tryPush(tmp);
				if (!ret)
					queue->arena.destroy<T>(tmp);
				return ret;
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
		}

		void pop(T &value)
		{
			T *tmp = nullptr;
			queue->pop((void*&)tmp);
			CAGE_ASSERT(tmp);
			try
			{
				value = templates::move(*tmp);
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
			queue->arena.destroy<T>(tmp);
		}

		bool tryPop(T &value)
		{
			T *tmp = nullptr;
			if (!queue->tryPop((void*&)tmp))
				return false;
			CAGE_ASSERT(tmp);
			try
			{
				value = templates::move(*tmp);
			}
			catch (...)
			{
				queue->arena.destroy<T>(tmp);
				throw;
			}
			queue->arena.destroy<T>(tmp);
			return true;
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
