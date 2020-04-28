#ifndef guard_rectPacking_h_CBAB7F4B_90B1_4151_968F_9C5336718D0D
#define guard_rectPacking_h_CBAB7F4B_90B1_4151_968F_9C5336718D0D

#include "core.h"

namespace cage
{
	class CAGE_CORE_API RectPacking : private Immovable
	{
	public:
		void add(uint32 id, uint32 width, uint32 height);
		bool solve(uint32 width, uint32 height);
		void get(uint32 index, uint32 &id, uint32 &x, uint32 &y) const;
		uint32 count() const;
	};

	struct CAGE_CORE_API RectPackingCreateConfig
	{
		uint32 margin = 0;
	};

	CAGE_CORE_API Holder<RectPacking> newRectPacking(const RectPackingCreateConfig &config);
}

#endif // guard_rectPacking_h_CBAB7F4B_90B1_4151_968F_9C5336718D0D
