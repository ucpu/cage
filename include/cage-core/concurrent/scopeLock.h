#ifndef guard_scopeLock_h_FD550C1B142247939398ED038581B06B
#define guard_scopeLock_h_FD550C1B142247939398ED038581B06B

namespace cage
{
	template<class T>
	struct scopeLock
	{
		scopeLock(holder<T> &ptr, bool tryLock) : scopeLock(ptr.get(), tryLock)
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

		scopeLock(holder<T> &ptr) : scopeLock(ptr.get())
		{}

		scopeLock(T *ptr) : ptr(ptr)
		{
			ptr->lock();
		}

		~scopeLock()
		{
			if (ptr)
				ptr->unlock();
		}

		operator bool() const
		{
			return !!ptr;
		}

	private:
		T *ptr;

		friend class conditionalBaseClass;
	};
}

#endif // guard_scopeLock_h_FD550C1B142247939398ED038581B06B
