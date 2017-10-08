#ifndef guard_threadSafeQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
#define guard_threadSafeQueue_h_F17509C840DB4228AF89C97FCD8EC1E5

namespace cage
{
	template struct CAGE_API delegate<bool(void *)>;

	class CAGE_API threadSafeQueueClass
	{
	public:
		void push(void *value);
		void *pull();
		void *check(delegate<bool(void *)> checker);

		uint32 unsafeSize() const;
	};

	CAGE_API holder<threadSafeQueueClass> newThreadSafeQueue(uintPtr memory = 1024 * 1024);
}

#endif // guard_threadSafeQueue_h_F17509C840DB4228AF89C97FCD8EC1E5
