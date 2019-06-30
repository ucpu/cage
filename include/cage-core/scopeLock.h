#ifndef guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
#define guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444

namespace cage
{
	template<class T>
	struct scopeLock
	{
		scopeLock(holder<T> &ptr, bool tryLock) : scopeLock(ptr.get(), tryLock)
		{}

		scopeLock(holder<T> &ptr) : scopeLock(ptr.get())
		{}

		scopeLock(T *ptr, bool tryLock) : ptr(ptr)
		{
			if (tryLock)
			{
				if (!ptr->tryLock())
					this->ptr = nullptr;
			}
			else
				ptr->lock();
		}

		scopeLock(T *ptr) : ptr(ptr)
		{
			ptr->lock();
		}

		// non copyable
		scopeLock(const scopeLock &) = delete;
		scopeLock &operator = (const scopeLock &) = delete;

		// movable
		scopeLock(scopeLock &&other) noexcept : ptr(other.ptr)
		{
			other.ptr = nullptr;
		}

		scopeLock &operator = (scopeLock &&other)
		{
			if (ptr)
				ptr->unlock();
			ptr = other.ptr;
			other.ptr = nullptr;
		}

		~scopeLock()
		{
			if (ptr)
				ptr->unlock();
		}

		explicit operator bool() const noexcept
		{
			return !!ptr;
		}

	private:
		T *ptr;

		friend class conditionalBaseClass;
	};
}

#endif // guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
