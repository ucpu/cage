#ifndef guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
#define guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API FpsCamera : private Immovable
	{
	public:
		vec2 turningSpeed = vec2(0.008);
		real rollSpeed = 10;
		real movementSpeed = 1;
		rads pitchLimitUp = degs(80);
		rads pitchLimitDown = degs(-80);
		MouseButtonsFlags mouseButton = MouseButtonsFlags::None; // -1 for disabled, 0 for always
		bool keysEqEnabled = true;
		bool keysWsadEnabled = true;
		bool keysArrowsEnabled = true;
		bool freeMove = false; // true -> like a space ship (ignores pitch limits), false -> fps (uses pitch limits)

		void setEntity(Entity *ent = nullptr);
	};

	CAGE_ENGINE_API Holder<FpsCamera> newFpsCamera(Entity *ent = nullptr);
}

#endif // guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
