#ifndef guard_scopeLock_h_FD550C1B142247939398ED038581B06B
#define guard_scopeLock_h_FD550C1B142247939398ED038581B06B

namespace cage
{
	template<class T>
	struct scopeLock
	{
		scopeLock(holder<T> &ptr, int tryLock) : scopeLock(ptr.get(), tryLock)
		{}

		scopeLock(T *ptr, int tryLock) : ptr(ptr)
		{
			if (!ptr->tryLock())
				ptr = nullptr;
		}

		scopeLock(holder<T> &ptr) : scopeLock(ptr.get())
		{}

		scopeLock(T *ptr) : ptr(ptr)
		{
			ptr->lock();
		}

		~scopeLock()
		{
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
