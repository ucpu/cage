#ifndef guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
#define guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A

namespace cage
{
	class CAGE_API fpsCamera : private Immovable
	{
	public:
		vec2 turningSpeed;
		real wheelSpeed;
		real movementSpeed;
		rads pitchLimitUp;
		rads pitchLimitDown;
		mouseButtonsFlags mouseButton; // -1 for disabled, 0 for always
		bool keysEqEnabled;
		bool keysWsadEnabled;
		bool keysArrowsEnabled;
		bool freeMove; // true -> like a space ship (ignores pitch limits), false -> fps (uses pitch limits)

		fpsCamera();
		void setEntity(Entity *ent = nullptr);
	};

	CAGE_API Holder<fpsCamera> newFpsCamera(Entity *ent = nullptr);
}

#endif // guard_fps_camera_h_E21CB47685994BD39846DDB714AF034A
