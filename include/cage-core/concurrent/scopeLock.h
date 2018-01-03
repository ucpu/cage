#ifndef guard_scopeLock_h_FD550C1B142247939398ED038581B06B
#define guard_scopeLock_h_FD550C1B142247939398ED038581B06B

namespace cage
{
	template<class T> struct scopeLock
	{
		scopeLock(holder<T> &ptr) : ptr(ptr.get())
		{
			ptr->lock();
		}

		scopeLock(T *ptr) : ptr(ptr)
		{
			ptr->lock();
		}

		~scopeLock()
		{
			ptr->unlock();
		}

	private:
		T *ptr;

		friend class conditionalBaseClass;
	};
}

#endif // guard_scopeLock_h_FD550C1B142247939398ED038581B06B
