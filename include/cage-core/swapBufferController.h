#ifndef guard_swapBufferController_h_rds4jh4jdr64jzdr64
#define guard_swapBufferController_h_rds4jh4jdr64jzdr64

namespace cage
{
	/*
	holder<swapBufferControllerClass> controller = newSwapBufferController();
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
		class CAGE_API swapBufferLock
		{
		public:
			swapBufferLock();
			swapBufferLock(swapBufferControllerClass *controller, uint32 index);
			swapBufferLock(const swapBufferLock &) = delete; // non-copyable
			swapBufferLock(swapBufferLock &&other); // movable
			~swapBufferLock();
			swapBufferLock &operator = (const swapBufferLock &) = delete; // non-copyable
			swapBufferLock &operator = (swapBufferLock &&other); // movable
			explicit operator bool() const { return !!controller_; }
			uint32 index() const { CAGE_ASSERT_RUNTIME(!!controller_); return index_; }

		private:
			swapBufferControllerClass *controller_;
			uint32 index_;
		};
	}

	class CAGE_API swapBufferControllerClass
	{
	public:
		privat::swapBufferLock read();
		privat::swapBufferLock write();
	};

	struct CAGE_API swapBufferControllerCreateConfig
	{
		uint32 buffersCount;
		bool repeatedReads; // allow to read last buffer again (instead of failing) if the producer cannot keep up - this can lead to duplicated data, but it may safe some unnecessary copies
		bool repeatedWrites; // allow to override last write buffer (instead of failing) if the consumer cannot keep up - this allows to lose some data, but the consumer will get the most up-to-date data
		swapBufferControllerCreateConfig(uint32 buffersCount);
	};

	CAGE_API holder<swapBufferControllerClass> newSwapBufferController(const swapBufferControllerCreateConfig &config);
}

#endif // guard_swapBufferController_h_rds4jh4jdr64jzdr64
