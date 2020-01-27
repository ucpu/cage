#ifndef guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
#define guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API FpsCamera : private Immovable
	{
	public:
		vec2 turningSpeed;
		real wheelSpeed;
		real movementSpeed;
		rads pitchLimitUp;
		rads pitchLimitDown;
		MouseButtonsFlags mouseButton; // -1 for disabled, 0 for always
		bool keysEqEnabled;
		bool keysWsadEnabled;
		bool keysArrowsEnabled;
		bool freeMove; // true -> like a space ship (ignores pitch limits), false -> fps (uses pitch limits)

		FpsCamera();
		void setEntity(Entity *ent = nullptr);
	};

	CAGE_ENGINE_API Holder<FpsCamera> newFpsCamera(Entity *ent = nullptr);
}

#endif // guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
