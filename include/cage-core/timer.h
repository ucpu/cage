#ifndef guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_
#define guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_

namespace cage
{
	class CAGE_API timer : private immovable
	{
	public:
		void reset();
		uint64 microsSinceStart();
		uint64 microsSinceLast();
	};

	CAGE_API holder<timer> newTimer();

	namespace detail
	{
		CAGE_API void getSystemDateTime(uint32 &year, uint32 &month, uint32 &day, uint32 &hour, uint32 &minute, uint32 &second);
		CAGE_API string formatDateTime(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 minute, uint32 second);
	}
}

#endif // guard_timer_h_95034fdd_21d1_4b36_b8f9_d5eeb0f6e2c1_
