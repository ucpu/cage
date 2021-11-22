#include <cage-core/concurrent.h>
#include <cage-core/swapBufferGuard.h>

namespace cage
{
	namespace
	{
		enum class StateEnum
		{
			Nothing,
			Reading,
			Read,
			Writing,
			Wrote,
		};

		class SwapBufferGuardImpl : public SwapBufferGuard
		{
		public:
			explicit SwapBufferGuardImpl(const SwapBufferGuardCreateConfig &config) : states{ StateEnum::Nothing, StateEnum::Nothing, StateEnum::Nothing, StateEnum::Nothing }, ri(0), wi(0), buffersCount(config.buffersCount), repeatedReads(config.repeatedReads), repeatedWrites(config.repeatedWrites)
			{
				CAGE_ASSERT(buffersCount > 1 && buffersCount < 5);
				CAGE_ASSERT(buffersCount > 1u + repeatedReads + repeatedWrites);
				mutex = newMutex();
			}

			bool readable() const
			{
				for (uint32 i = 0; i < buffersCount; i++)
					if (states[i] == StateEnum::Reading)
						return false;
				return true;
			}

			bool writeable() const
			{
				for (uint32 i = 0; i < buffersCount; i++)
					if (states[i] == StateEnum::Writing)
						return false;
				return true;
			}

			privat::SwapBufferLock read()
			{
				ScopeLock lock(mutex);
				CAGE_ASSERT(readable());
				if (repeatedReads)
				{
					if (next(ri) != wi && states[next(ri)] == StateEnum::Wrote)
					{
						ri = next(ri);
						states[ri] = StateEnum::Reading;
						return privat::SwapBufferLock(this, ri);
					}
					if (states[ri] == StateEnum::Read)
					{
						states[ri] = StateEnum::Reading;
						return privat::SwapBufferLock(this, ri);
					}
				}
				else
				{
					if (states[next(ri)] == StateEnum::Wrote)
					{
						ri = next(ri);
						states[ri] = StateEnum::Reading;
						return privat::SwapBufferLock(this, ri);
					}
				}
				return {};
			}

			privat::SwapBufferLock write()
			{
				ScopeLock lock(mutex);
				CAGE_ASSERT(writeable());
				if (repeatedWrites)
				{
					if ((next(wi) != ri && states[next(wi)] == StateEnum::Read) || states[next(wi)] == StateEnum::Nothing)
					{
						wi = next(wi);
						states[wi] = StateEnum::Writing;
						return privat::SwapBufferLock(this, wi);
					}
					if (states[wi] == StateEnum::Wrote)
					{
						states[wi] = StateEnum::Writing;
						return privat::SwapBufferLock(this, wi);
					}
				}
				else
				{
					if (states[next(wi)] == StateEnum::Read || states[next(wi)] == StateEnum::Nothing)
					{
						wi = next(wi);
						states[wi] = StateEnum::Writing;
						return privat::SwapBufferLock(this, wi);
					}
				}
				return {};
			}

			void finished(uint32 index)
			{
				ScopeLock lock(mutex);
				switch (states[index])
				{
				case StateEnum::Reading:
					states[index] = StateEnum::Read;
					break;
				case StateEnum::Writing:
					states[index] = StateEnum::Wrote;
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid swap buffer controller state");
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

			Holder<Mutex> mutex;
			StateEnum states[4];
			uint32 ri, wi;
			const uint32 buffersCount;
			const bool repeatedReads, repeatedWrites;
		};
	}

	namespace privat
	{
		SwapBufferLock::SwapBufferLock()
		{}

		SwapBufferLock::SwapBufferLock(SwapBufferGuard *controller, uint32 index) : controller_(controller), index_(index)
		{
			SwapBufferGuardImpl *impl = (SwapBufferGuardImpl*)controller_;
			CAGE_ASSERT(impl->states[index] == StateEnum::Reading || impl->states[index] == StateEnum::Writing);
		}

		SwapBufferLock::SwapBufferLock(SwapBufferLock &&other) noexcept : controller_(nullptr), index_(m)
		{
			std::swap(controller_, other.controller_);
			std::swap(index_, other.index_);
		}

		SwapBufferLock::~SwapBufferLock()
		{
			if (!controller_)
				return;
			SwapBufferGuardImpl *impl = (SwapBufferGuardImpl*)controller_;
			impl->finished(index_);
		}

		SwapBufferLock &SwapBufferLock::operator = (SwapBufferLock &&other) noexcept
		{
			if (controller_)
			{
				SwapBufferGuardImpl *impl = (SwapBufferGuardImpl*)controller_;
				impl->finished(index_);
				controller_ = nullptr;
				index_ = m;
			}
			std::swap(controller_, other.controller_);
			std::swap(index_, other.index_);
			return *this;
		}
	}

	privat::SwapBufferLock SwapBufferGuard::read()
	{
		SwapBufferGuardImpl *impl = (SwapBufferGuardImpl*)this;
		return impl->read();
	}

	privat::SwapBufferLock SwapBufferGuard::write()
	{
		SwapBufferGuardImpl *impl = (SwapBufferGuardImpl*)this;
		return impl->write();
	}

	//SwapBufferGuardCreateConfig::SwapBufferGuardCreateConfig(uint32 buffersCount) : buffersCount(buffersCount)
	//{}

	Holder<SwapBufferGuard> newSwapBufferGuard(const SwapBufferGuardCreateConfig &config)
	{
		return systemMemory().createImpl<SwapBufferGuard, SwapBufferGuardImpl>(config);
	}
}

