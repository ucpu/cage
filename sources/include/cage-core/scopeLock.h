#ifndef guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
#define guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444

#include "core.h"

namespace cage
{
	struct TryLockTag {};
	struct ReadLockTag {};
	struct WriteLockTag {};

	template<class T>
	struct ScopeLock
	{
		explicit ScopeLock(const Holder<T> &ptr, TryLockTag) : ScopeLock(+ptr, TryLockTag())
		{}

		explicit ScopeLock(const Holder<T> &ptr, ReadLockTag) : ScopeLock(+ptr, ReadLockTag())
		{}

		explicit ScopeLock(const Holder<T> &ptr, WriteLockTag) : ScopeLock(+ptr, WriteLockTag())
		{}

		explicit ScopeLock(const Holder<T> &ptr) : ScopeLock(+ptr)
		{}

		explicit ScopeLock(T *ptr, TryLockTag) : ptr(ptr)
		{
			if (!ptr->tryLock())
				this->ptr = nullptr;
		}

		explicit ScopeLock(T *ptr, ReadLockTag) : ptr(ptr)
		{
			ptr->readLock();
		}

		explicit ScopeLock(T *ptr, WriteLockTag) : ptr(ptr)
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
