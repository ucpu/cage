#ifndef guard_screenList_h_q56ew4rk87h64847
#define guard_screenList_h_q56ew4rk87h64847

#include "core.h"

namespace cage
{
	struct CAGE_ENGINE_API ScreenMode
	{
	public:
		ivec2 resolution;
		uint32 frequency;
		ScreenMode();
	};

	class CAGE_ENGINE_API ScreenDevice : private Immovable
	{
	public:
		string id() const;
		string name() const;

		uint32 modesCount() const;
		uint32 currentMode() const;
		const ScreenMode &mode(uint32 index) const;
		PointerRange<const ScreenMode> modes() const;
	};

	class CAGE_ENGINE_API ScreenList : private Immovable
	{
	public:
		uint32 devicesCount() const;
		uint32 defaultDevice() const;
		const ScreenDevice *device(uint32 index) const;
		Holder<PointerRange<const ScreenDevice *>> devices() const;
	};

	CAGE_ENGINE_API Holder<ScreenList> newScreenList();
}

#endif // guard_screenList_h_q56ew4rk87h64847
