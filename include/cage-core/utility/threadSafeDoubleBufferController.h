#ifndef guard_threadSafeDoubleBufferController_h_rds4jh4jdr64jzdr64
#define guard_threadSafeDoubleBufferController_h_rds4jh4jdr64jzdr64

namespace cage
{
	/*
	holder<threadSafeDoubleBufferControllerClass> controller = newThreadSafeDoubleBufferController();
	// consummer thread
	while (running)
	{
		if (auto mark = controller->read())
		{
			// read the data from mark.index buffer
		}
		else
		{
			// use older data
		}
	}
	// producer thread
	while (running)
	{
		if (auto mark = controller->write())
		{
			// write new data to mark.index buffer
		}
		else
		{
			// skip writing these data and proceed to generating next frame
		}
	}
	*/

	namespace privat
	{
		class CAGE_API threadSafeDoubleBufferMark
		{
		public:
			threadSafeDoubleBufferMark(threadSafeDoubleBufferControllerClass *controller, uint32 index);
			threadSafeDoubleBufferMark(const threadSafeDoubleBufferMark &) = delete; // non-copyable
			threadSafeDoubleBufferMark(threadSafeDoubleBufferMark &&other); // moveable
			~threadSafeDoubleBufferMark();
			threadSafeDoubleBufferMark &operator = (const threadSafeDoubleBufferMark &) = delete; // non-copyable
			threadSafeDoubleBufferMark &operator = (threadSafeDoubleBufferMark &&other); // moveable
			operator bool() const { return !!controller; }

			threadSafeDoubleBufferControllerClass *controller;
			uint32 index;
		};
	}

	class CAGE_API threadSafeDoubleBufferControllerClass
	{
	public:
		privat::threadSafeDoubleBufferMark read();
		privat::threadSafeDoubleBufferMark write();
	};

	CAGE_API holder<threadSafeDoubleBufferControllerClass> newThreadSafeDoubleBufferController();
}

#endif // guard_threadSafeDoubleBufferController_h_rds4jh4jdr64jzdr64
