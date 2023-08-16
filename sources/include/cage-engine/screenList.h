#ifndef guard_screenList_h_q56ew4rk87h64847
#define guard_screenList_h_q56ew4rk87h64847

#include <cage-engine/core.h>

namespace cage
{
	struct CAGE_ENGINE_API ScreenMode
	{
	public:
		Vec2i resolution;
		uint32 frequency = 0;
	};

	class CAGE_ENGINE_API ScreenDevice : private Immovable
	{
	public:
		String id() const;
		String name() const;

		uint32 currentMode() const;
		PointerRange<const ScreenMode> modes() const;
	};

	class CAGE_ENGINE_API ScreenList : private Immovable
	{
	public:
		uint32 defaultDevice() const;
		Holder<PointerRange<const ScreenDevice *>> devices() const;
	};

	CAGE_ENGINE_API Holder<ScreenList> newScreenList();
}

#endif // guard_screenList_h_q56ew4rk87h64847
