#ifndef guard_threadSafeSwapBufferController_h_rds4jh4jdr64jzdr64
#define guard_threadSafeSwapBufferController_h_rds4jh4jdr64jzdr64

namespace cage
{
	/*
	holder<threadSafeSwapBufferControllerClass> controller = newThreadSafeSwapBufferController();
	// consumer thread
	while (running)
	{
		if (auto lock = controller->read())
		{
			// read the data from lock.index() buffer
		}
		else
		{
			// the producer cannot keep up
		}
	}
	// producer thread
	while (running)
	{
		if (auto lock = controller->write())
		{
			// write new data to lock.index() buffer
		}
		else
		{
			// the consumer cannot keep up
		}
	}
	*/

	namespace privat
	{
		class CAGE_API threadSafeSwapBufferLock
		{
		public:
			threadSafeSwapBufferLock();
			threadSafeSwapBufferLock(threadSafeSwapBufferControllerClass *controller, uint32 index);
			threadSafeSwapBufferLock(const threadSafeSwapBufferLock &) = delete; // non-copyable
			threadSafeSwapBufferLock(threadSafeSwapBufferLock &&other); // moveable
			~threadSafeSwapBufferLock();
			threadSafeSwapBufferLock &operator = (const threadSafeSwapBufferLock &) = delete; // non-copyable
			threadSafeSwapBufferLock &operator = (threadSafeSwapBufferLock &&other); // moveable
			operator bool() const { return !!controller_; }
			uint32 index() const { CAGE_ASSERT_RUNTIME(!!controller_); return index_; }

		private:
			threadSafeSwapBufferControllerClass *controller_;
			uint32 index_;
		};
	}

	class CAGE_API threadSafeSwapBufferControllerClass
	{
	public:
		privat::threadSafeSwapBufferLock read();
		privat::threadSafeSwapBufferLock write();
	};

	struct CAGE_API threadSafeSwapBufferControllerCreateConfig
	{
		uint32 buffersCount;
		bool repeatedReads; // allow to read last buffer again (instead of failing) if the producer cannot keep up - this can lead to duplicated data, but it may safe some unnecessary copies
		bool repeatedWrites; // allow to override last write buffer (instead of failing) if the consumer cannot keep up - this allows to lose some data, but the consumer will get the most up-to-date data
		threadSafeSwapBufferControllerCreateConfig(uint32 buffersCount);
	};

	CAGE_API holder<threadSafeSwapBufferControllerClass> newThreadSafeSwapBufferController(const threadSafeSwapBufferControllerCreateConfig &config);
}

#endif // guard_threadSafeSwapBufferController_h_rds4jh4jdr64jzdr64
