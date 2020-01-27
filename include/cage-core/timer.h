#ifndef guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_
#define guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_

#include "core.h"

namespace cage
{
	class CAGE_CORE_API Timer : private Immovable
	{
	public:
		void reset();
		uint64 microsSinceStart();
		uint64 microsSinceLast();
	};

	CAGE_CORE_API Holder<Timer> newTimer();

	namespace detail
	{
		CAGE_CORE_API void getSystemDateTime(uint32 &year, uint32 &month, uint32 &day, uint32 &hour, uint32 &minute, uint32 &second);
		CAGE_CORE_API string formatDateTime(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 minute, uint32 second);
	}
}

#endif // guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_
