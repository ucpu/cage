#ifndef header_guard_interpolationTimingCorrector_h_nbvcxwer78d
#define header_guard_interpolationTimingCorrector_h_nbvcxwer78d

#include <cage-core/math.h>
#include <cage-core/variableSmoothingBuffer.h>

namespace cage
{
	struct InterpolationTimingCorrector
	{
		uint64 operator() (uint64 emit, uint64 dispatch, uint64 step)
		{
			CAGE_ASSERT(step > 0);
			corrections.add((sint64)emit - (sint64)dispatch);
			const sint64 c = corrections.smooth();
			return max(emit, dispatch + c + step / 2);
		}

		VariableSmoothingBuffer<sint64, 60> corrections;
	};
}

#endif
