#ifndef guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
#define guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444

#include "core.h"

namespace cage
{
	struct TryLock {};
	struct ReadLock {};
	struct WriteLock {};

	template<class T>
	struct ScopeLock
	{
		explicit ScopeLock(const Holder<T> &ptr, TryLock) : ScopeLock(ptr.get(), TryLock())
		{}

		explicit ScopeLock(const Holder<T> &ptr, ReadLock) : ScopeLock(ptr.get(), ReadLock())
		{}

		explicit ScopeLock(const Holder<T> &ptr, WriteLock) : ScopeLock(ptr.get(), WriteLock())
		{}

		explicit ScopeLock(const Holder<T> &ptr) : ScopeLock(ptr.get())
		{}

		explicit ScopeLock(T *ptr, TryLock) : ptr(ptr)
		{
			if (!ptr->tryLock())
				this->ptr = nullptr;
		}

		explicit ScopeLock(T *ptr, ReadLock) : ptr(ptr)
		{
			ptr->readLock();
		}

		explicit ScopeLock(T *ptr, WriteLock) : ptr(ptr)
		{
			ptr->writeLock();
		}

		explicit ScopeLock(T *ptr) : ptr(ptr)
		{
			ptr->lock();
		}

		// non copyable
		ScopeLock(const ScopeLock &) = delete;
		ScopeLock &operator = (const ScopeLock &) = delete;

		// move constructible
		ScopeLock(ScopeLock &&other) noexcept : ptr(other.ptr)
		{
			other.ptr = nullptr;
		}

		// not move assignable (releasing the original lock owned by this would not be atomic)
		ScopeLock &operator = (ScopeLock &&) = delete;

		~ScopeLock()
		{
			clear();
		}

		void clear()
		{
			if (ptr)
			{
				ptr->unlock();
				ptr = nullptr;
			}
		}

		explicit operator bool() const noexcept
		{
			return !!ptr;
		}

	private:
		T *ptr = nullptr;

		friend class ConditionalVariableBase;
	};
}

#endif // guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
