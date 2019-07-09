#ifndef guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
#define guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444

namespace cage
{
	template<class T>
	struct scopeLock
	{
		explicit scopeLock(holder<T> &ptr, bool tryLock) : scopeLock(ptr.get(), tryLock)
		{}

		explicit scopeLock(holder<T> &ptr) : scopeLock(ptr.get())
		{}

		explicit scopeLock(T *ptr, bool tryLock) : ptr(ptr)
		{
			if (tryLock)
			{
				if (!ptr->tryLock())
					this->ptr = nullptr;
			}
			else
				ptr->lock();
		}

		explicit scopeLock(T *ptr) : ptr(ptr)
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
			clear();
			ptr = other.ptr;
			other.ptr = nullptr;
		}

		~scopeLock()
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
		T *ptr;

		friend class syncConditionalBase;
	};
}

#endif // guard_scopeLock_h_sdgfrdsfhg54s5sd54hg544h444
