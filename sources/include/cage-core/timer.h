#ifndef guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_
#define guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_

#include <cage-core/core.h>

namespace cage
{
	class CAGE_CORE_API Timer : private Immovable
	{
	public:
		void reset();
		uint64 duration(); // microseconds since construction, or last call to reset, whichever is smaller
		uint64 elapsed(); // microseconds since construction, last call to reset, or last call to elapsed, whichever is smallest
	};

	CAGE_CORE_API Holder<Timer> newTimer();
}

#endif // guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_
