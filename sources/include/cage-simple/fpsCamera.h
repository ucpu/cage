#ifndef guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
#define guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A

#include <cage-engine/core.h>

namespace cage
{
	class FpsCamera : private Immovable
	{
	public:
		Vec2 turningSpeed = Vec2(0.008);
		Real rollSpeed = 10;
		Real movementSpeed = 1;
		Rads pitchLimitUp = Degs(80);
		Rads pitchLimitDown = Degs(-80);
		MouseButtonsFlags mouseButton = MouseButtonsFlags::None; // -1 for disabled, 0 for always
		bool keysEqEnabled = true;
		bool keysWsadEnabled = true;
		bool keysArrowsEnabled = true;
		bool freeMove = false; // true -> like a space ship (ignores pitch limits), false -> fps (uses pitch limits)

		void setEntity(Entity *ent = nullptr);
	};

	Holder<FpsCamera> newFpsCamera(Entity *ent = nullptr);
}

#endif // guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
