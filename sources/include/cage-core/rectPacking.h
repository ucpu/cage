#ifndef guard_rectPacking_h_CBAB7F4B_90B1_4151_968F_9C5336718D0D
#define guard_rectPacking_h_CBAB7F4B_90B1_4151_968F_9C5336718D0D

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API PackingRect
	{
		uint32 id = 0;
		uint32 width = 0;
		uint32 height = 0;
		uint32 x = 0;
		uint32 y = 0;
	};

	struct CAGE_CORE_API RectPackingSolveConfig
	{
		uint32 width = 0;
		uint32 height = 0;
		uint32 margin = 0;
	};

	class CAGE_CORE_API RectPacking : private Immovable
	{
	public:
		void resize(uint32 cnt);
		PointerRange<PackingRect> data();
		PointerRange<const PackingRect> data() const;

		bool solve(const RectPackingSolveConfig &config);
	};

	CAGE_CORE_API Holder<RectPacking> newRectPacking();
}

#endif // guard_rectPacking_h_CBAB7F4B_90B1_4151_968F_9C5336718D0D
