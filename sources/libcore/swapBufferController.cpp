#include <utility>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/swapBufferController.h>

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

		class swapBufferControllerImpl : public swapBufferControllerClass
		{
		public:
			swapBufferControllerImpl(const swapBufferControllerCreateConfig &config) : states{ stateEnum::Nothing, stateEnum::Nothing, stateEnum::Nothing, stateEnum::Nothing }, ri(0), wi(0), buffersCount(config.buffersCount), repeatedReads(config.repeatedReads), repeatedWrites(config.repeatedWrites)
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

			privat::swapBufferLock read()
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

			privat::swapBufferLock write()
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
		swapBufferLock::swapBufferLock() : controller_(nullptr), index_(m)
		{}

		swapBufferLock::swapBufferLock(swapBufferControllerClass *controller, uint32 index) : controller_(controller), index_(index)
		{
			swapBufferControllerImpl *impl = (swapBufferControllerImpl*)controller_;
			CAGE_ASSERT_RUNTIME(impl->states[index] == stateEnum::Reading || impl->states[index] == stateEnum::Writing);
		}

		swapBufferLock::swapBufferLock(swapBufferLock &&other) : controller_(nullptr), index_(m)
		{
			std::swap(controller_, other.controller_);
			std::swap(index_, other.index_);
		}

		swapBufferLock::~swapBufferLock()
		{
			if (!controller_)
				return;
			swapBufferControllerImpl *impl = (swapBufferControllerImpl*)controller_;
			impl->finished(index_);
		}

		swapBufferLock &swapBufferLock::operator = (swapBufferLock &&other)
		{
			if (controller_)
			{
				swapBufferControllerImpl *impl = (swapBufferControllerImpl*)controller_;
				impl->finished(index_);
				controller_ = nullptr;
				index_ = m;
			}
			std::swap(controller_, other.controller_);
			std::swap(index_, other.index_);
			return *this;
		}
	}

	privat::swapBufferLock swapBufferControllerClass::read()
	{
		swapBufferControllerImpl *impl = (swapBufferControllerImpl*)this;
		return impl->read();
	}

	privat::swapBufferLock swapBufferControllerClass::write()
	{
		swapBufferControllerImpl *impl = (swapBufferControllerImpl*)this;
		return impl->write();
	}

	swapBufferControllerCreateConfig::swapBufferControllerCreateConfig(uint32 buffersCount) : buffersCount(buffersCount), repeatedReads(false), repeatedWrites(false)
	{}

	holder<swapBufferControllerClass> newSwapBufferController(const swapBufferControllerCreateConfig &config)
	{
		return detail::systemArena().createImpl<swapBufferControllerClass, swapBufferControllerImpl>(config);
	}
}

