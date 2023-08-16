#ifndef guard_swapBufferController_h_rds4jh4jdr64jzdr64
#define guard_swapBufferController_h_rds4jh4jdr64jzdr64

#include <cage-core/core.h>

namespace cage
{
	/*
	Holder<SwapBufferGuard> controller = newSwapBufferGuard();

	// consumer thread
	while (running)
	{
		if (auto lock = controller->read())
		{
			// read data from lock.index() buffer
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

	class SwapBufferGuard;

	namespace privat
	{
		class CAGE_CORE_API SwapBufferLock : private Noncopyable
		{
		public:
			SwapBufferLock();
			explicit SwapBufferLock(SwapBufferGuard *controller, uint32 index);
			SwapBufferLock(SwapBufferLock &&other) noexcept;
			~SwapBufferLock();
			SwapBufferLock &operator=(SwapBufferLock &&other) noexcept;
			explicit operator bool() const { return !!controller_; }
			uint32 index() const
			{
				CAGE_ASSERT(!!controller_);
				return index_;
			}

		private:
			SwapBufferGuard *controller_ = nullptr;
			uint32 index_ = m;
		};
	}

	class CAGE_CORE_API SwapBufferGuard : private Immovable
	{
	public:
		[[nodiscard]] privat::SwapBufferLock read();
		[[nodiscard]] privat::SwapBufferLock write();
	};

	struct CAGE_CORE_API SwapBufferGuardCreateConfig
	{
		uint32 buffersCount = 0;
		bool repeatedReads = false; // allow to read last buffer again (instead of failing) if the producer cannot keep up - this can lead to duplicated data, but it may safe some unnecessary copies
		bool repeatedWrites = false; // allow to override last write buffer (instead of failing) if the consumer cannot keep up - this allows to lose some data, but the consumer will get the most up-to-date data
	};

	CAGE_CORE_API Holder<SwapBufferGuard> newSwapBufferGuard(const SwapBufferGuardCreateConfig &config);
}

#endif // guard_swapBufferController_h_rds4jh4jdr64jzdr64
