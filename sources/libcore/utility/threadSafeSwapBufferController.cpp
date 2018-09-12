#include <utility>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadSafeSwapBufferController.h>

namespace cage
{
	namespace
	{
		enum class stateEnum
		{
			Nothing,
			Reading,
			Read,
			Writing,
			Wrote,
		};

		class threadSafeSwapBufferControllerImpl : public threadSafeSwapBufferControllerClass
		{
		public:
			threadSafeSwapBufferControllerImpl(const threadSafeSwapBufferControllerCreateConfig &config) : states{ stateEnum::Nothing, stateEnum::Nothing, stateEnum::Nothing, stateEnum::Nothing }, ri(0), wi(0), buffersCount(config.buffersCount), repeatedReads(config.repeatedReads), repeatedWrites(config.repeatedWrites)
			{
				CAGE_ASSERT_RUNTIME(buffersCount > 1 && buffersCount < 5);
				CAGE_ASSERT_RUNTIME(buffersCount > 1u + repeatedReads + repeatedWrites);
				mutex = newMutex();
			}

			bool readable() const
			{
				for (uint32 i = 0; i < buffersCount; i++)
					if (states[i] == stateEnum::Reading)
						return false;
				return true;
			}

			bool writeable() const
			{
				for (uint32 i = 0; i < buffersCount; i++)
					if (states[i] == stateEnum::Writing)
						return false;
				return true;
			}

			privat::threadSafeSwapBufferLock read()
			{
				scopeLock<mutexClass> lock(mutex);
				CAGE_ASSERT_RUNTIME(readable(), "one reading at a time only");
				if (repeatedReads)
				{
					if (next(ri) != wi && states[next(ri)] == stateEnum::Wrote)
					{
						ri = next(ri);
						states[ri] = stateEnum::Reading;
						return { this, ri };
					}
					if (states[ri] == stateEnum::Read)
					{
						states[ri] = stateEnum::Reading;
						return { this, ri };
					}
				}
				else
				{
					if (states[next(ri)] == stateEnum::Wrote)
					{
						ri = next(ri);
						states[ri] = stateEnum::Reading;
						return { this, ri };
					}
				}
				return {};
			}

			privat::threadSafeSwapBufferLock write()
			{
				scopeLock<mutexClass> lock(mutex);
				CAGE_ASSERT_RUNTIME(writeable(), "one writing at a time only");
				if (repeatedWrites)
				{
					if ((next(wi) != ri && states[next(wi)] == stateEnum::Read) || states[next(wi)] == stateEnum::Nothing)
					{
						wi = next(wi);
						states[wi] = stateEnum::Writing;
						return { this, wi };
					}
					if (states[wi] == stateEnum::Wrote)
					{
						states[wi] = stateEnum::Writing;
						return { this, wi };
					}
				}
				else
				{
					if (states[next(wi)] == stateEnum::Read || states[next(wi)] == stateEnum::Nothing)
					{
						wi = next(wi);
						states[wi] = stateEnum::Writing;
						return { this, wi };
					}
				}
				return {};
			}

			void finished(uint32 index)
			{
				scopeLock<mutexClass> lock(mutex);
				switch (states[index])
				{
				case stateEnum::Reading:
					states[index] = stateEnum::Read;
					break;
				case stateEnum::Writing:
					states[index] = stateEnum::Wrote;
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid swap buffer controller state");
				}
			}

			uint32 next(uint32 i) const
			{
				return (i + 1) % buffersCount;
			}

			uint32 prev(uint32 i) const
			{
				return (i + buffersCount - 1) % buffersCount;
			}

			holder<mutexClass> mutex;
			stateEnum states[4];
			uint32 ri, wi;
			const uint32 buffersCount;
			const bool repeatedReads, repeatedWrites;
		};
	}

	namespace privat
	{
		threadSafeSwapBufferLock::threadSafeSwapBufferLock() : controller_(nullptr), index_(-1)
		{}

		threadSafeSwapBufferLock::threadSafeSwapBufferLock(threadSafeSwapBufferControllerClass *controller, uint32 index) : controller_(controller), index_(index)
		{
			threadSafeSwapBufferControllerImpl *impl = (threadSafeSwapBufferControllerImpl*)controller_;
			CAGE_ASSERT_RUNTIME(impl->states[index] == stateEnum::Reading || impl->states[index] == stateEnum::Writing);
		}

		threadSafeSwapBufferLock::threadSafeSwapBufferLock(threadSafeSwapBufferLock &&other) : controller_(nullptr), index_(-1)
		{
			std::swap(controller_, other.controller_);
			std::swap(index_, other.index_);
		}

		threadSafeSwapBufferLock::~threadSafeSwapBufferLock()
		{
			if (!controller_)
				return;
			threadSafeSwapBufferControllerImpl *impl = (threadSafeSwapBufferControllerImpl*)controller_;
			impl->finished(index_);
		}

		threadSafeSwapBufferLock &threadSafeSwapBufferLock::operator = (threadSafeSwapBufferLock &&other)
		{
			if (controller_)
			{
				threadSafeSwapBufferControllerImpl *impl = (threadSafeSwapBufferControllerImpl*)controller_;
				impl->finished(index_);
				controller_ = nullptr;
				index_ = -1;
			}
			std::swap(controller_, other.controller_);
			std::swap(index_, other.index_);
			return *this;
		}
	}

	privat::threadSafeSwapBufferLock threadSafeSwapBufferControllerClass::read()
	{
		threadSafeSwapBufferControllerImpl *impl = (threadSafeSwapBufferControllerImpl*)this;
		return impl->read();
	}

	privat::threadSafeSwapBufferLock threadSafeSwapBufferControllerClass::write()
	{
		threadSafeSwapBufferControllerImpl *impl = (threadSafeSwapBufferControllerImpl*)this;
		return impl->write();
	}

	threadSafeSwapBufferControllerCreateConfig::threadSafeSwapBufferControllerCreateConfig(uint32 buffersCount) : buffersCount(buffersCount), repeatedReads(false), repeatedWrites(false)
	{}

	holder<threadSafeSwapBufferControllerClass> newThreadSafeSwapBufferController(const threadSafeSwapBufferControllerCreateConfig &config)
	{
		return detail::systemArena().createImpl<threadSafeSwapBufferControllerClass, threadSafeSwapBufferControllerImpl>(config);
	}
}

