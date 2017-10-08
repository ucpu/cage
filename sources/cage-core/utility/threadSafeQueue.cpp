#include <list>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadSafeQueue.h>

namespace cage
{
	namespace
	{
		class threadSafeQueueImpl : public threadSafeQueueClass
		{
		public:
			threadSafeQueueImpl(uintPtr memory) : pool(memory), queue(memoryArena(&pool))
			{
				mutex = newMutex();
			}

			memoryArenaGrowing<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeList<void*>)>, memoryConcurrentPolicyNone> pool;
			std::list <void*, memoryArenaStd<> > queue;
			holder<mutexClass> mutex;
		};
	}

	void threadSafeQueueClass::push(void *value)
	{
		threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
		scopeLock <mutexClass> l(impl->mutex);
		impl->queue.push_front(value);
	}

	void *threadSafeQueueClass::pull()
	{
		threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
		scopeLock <mutexClass> l(impl->mutex);
		if (impl->queue.empty())
			return nullptr;
		void *res = impl->queue.back();
		impl->queue.pop_back();
		return res;
	}

	void *threadSafeQueueClass::check(delegate<bool(void *)> checker)
	{
		threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
		scopeLock <mutexClass> l(impl->mutex);
		if (impl->queue.empty())
			return nullptr;
		void *res = impl->queue.back();
		if (checker(res))
		{
			impl->queue.pop_back();
			return res;
		}
		return nullptr;
	}

	uint32 threadSafeQueueClass::unsafeSize() const
	{
		threadSafeQueueImpl *impl = (threadSafeQueueImpl *)this;
		return numeric_cast<uint32>(impl->queue.size());
	}

	holder<threadSafeQueueClass> newThreadSafeQueue(uintPtr memory)
	{
		return detail::systemArena().createImpl <threadSafeQueueClass, threadSafeQueueImpl>(memory);
	}
}