#include <utility>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadSafeDoubleBufferController.h>

namespace cage
{
	namespace
	{
		class threadSafeDoubleBufferControllerImpl : public threadSafeDoubleBufferControllerClass
		{
		public:
			threadSafeDoubleBufferControllerImpl() : states{1, 1}, currentRead(0), currentWrite(0)
			{
				mutex = newMutex();
			}

			privat::threadSafeDoubleBufferMark read()
			{
				scopeLock<mutexClass> lock(mutex);
				if (states[currentRead] == 3)
				{
					states[currentRead] = 4;
					return privat::threadSafeDoubleBufferMark(this, currentRead);
				}
				return privat::threadSafeDoubleBufferMark(nullptr, -1);
			}

			privat::threadSafeDoubleBufferMark write()
			{
				scopeLock<mutexClass> lock(mutex);
				if (states[currentWrite] == 1)
				{
					states[currentWrite] = 2;
					return privat::threadSafeDoubleBufferMark(this, currentWrite);
				}
				return privat::threadSafeDoubleBufferMark(nullptr, -1);
			}

			void finished(uint32 index)
			{
				scopeLock<mutexClass> lock(mutex);
				switch (states[index])
				{
				case 2: // finished writing
					states[index] = 3; // enable reading
					currentWrite = !currentWrite;
					break;
				case 4: // finished reading
					states[index] = 1; // enable writing
					currentRead = !currentRead;
					break;
				default:
					CAGE_ASSERT_RUNTIME(false, "invalid state in threadSafeDoubleBufferController");
				}
			}

			holder<mutexClass> mutex;
			uint32 states[2];
			uint32 currentRead, currentWrite;
		};
	}

	namespace privat
	{
		threadSafeDoubleBufferMark::threadSafeDoubleBufferMark(threadSafeDoubleBufferControllerClass *controller, uint32 index) : controller(controller), index(index)
		{}

		threadSafeDoubleBufferMark::threadSafeDoubleBufferMark(threadSafeDoubleBufferMark &&other) : controller(other.controller), index(other.index)
		{
			other.controller = nullptr;
			other.index = -1;
		}

		threadSafeDoubleBufferMark::~threadSafeDoubleBufferMark()
		{
			if (!controller)
				return;
			threadSafeDoubleBufferControllerImpl *impl = (threadSafeDoubleBufferControllerImpl*)controller;
			impl->finished(index);
		}

		threadSafeDoubleBufferMark &threadSafeDoubleBufferMark::operator = (threadSafeDoubleBufferMark &&other)
		{
			if (controller)
			{
				threadSafeDoubleBufferControllerImpl *impl = (threadSafeDoubleBufferControllerImpl*)controller;
				impl->finished(index);
				controller = nullptr;
				index = -1;
			}
			std::swap(controller, other.controller);
			std::swap(index, other.index);
			return *this;
		}
	}

	privat::threadSafeDoubleBufferMark threadSafeDoubleBufferControllerClass::read()
	{
		threadSafeDoubleBufferControllerImpl *impl = (threadSafeDoubleBufferControllerImpl*)this;
		return impl->read();
	}

	privat::threadSafeDoubleBufferMark threadSafeDoubleBufferControllerClass::write()
	{
		threadSafeDoubleBufferControllerImpl *impl = (threadSafeDoubleBufferControllerImpl*)this;
		return impl->write();
	}

	holder<threadSafeDoubleBufferControllerClass> newThreadSafeDoubleBufferController()
	{
		return detail::systemArena().createImpl<threadSafeDoubleBufferControllerClass, threadSafeDoubleBufferControllerImpl>();
	}
}

