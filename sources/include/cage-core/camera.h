#ifndef guard_camera_h_m1nv5e8967gsx856dghzgg
#define guard_camera_h_m1nv5e8967gsx856dghzgg

#include "math.h"

namespace cage
{
	CAGE_CORE_API Mat4 lookAt(const Vec3 &eye, const Vec3 &target, const Vec3 &up);

	// fov is along vertical axis
	CAGE_CORE_API Mat4 perspectiveProjection(Rads fov, Real aspectRatio, Real near, Real far);
	CAGE_CORE_API Mat4 perspectiveProjection(Rads fov, Real aspectRatio, Real near, Real far, Real zeroParallaxDistance, Real eyeSeparation); // use negative eyeSeparation for left eye
	CAGE_CORE_API Mat4 perspectiveProjection(Real left, Real right, Real bottom, Real top, Real near, Real far);
	CAGE_CORE_API Mat4 orthographicProjection(Real left, Real right, Real bottom, Real top, Real near, Real far);

	enum class StereoModeEnum : uint32
	{
		Mono,
		Horizontal,
		Vertical,
		Separate,
	};
	CAGE_CORE_API StringPointer stereoModeToString(StereoModeEnum mode);
	CAGE_CORE_API StereoModeEnum stringToStereoMode(const String &mode);

	enum class StereoEyeEnum : uint32
	{
		Mono,
		Left,
		Right,
	};

	struct CAGE_CORE_API StereoCameraInput : public Transform
	{
		Vec2 viewportOrigin; // 0 .. 1
		Vec2 viewportSize = Vec2(1); // 0 .. 1
		Real aspectRatio;
		Rads fov;
		Real near;
		Real far;
		Real zeroParallaxDistance;
		Real eyeSeparation;
		bool orthographic = false;
	};
	struct CAGE_CORE_API StereoCameraOutput
	{
		Mat4 view;
		Mat4 projection;
		Vec2 viewportOrigin; // 0 .. 1
		Vec2 viewportSize; // 0 .. 1
	};
	CAGE_CORE_API StereoCameraOutput stereoCamera(const StereoCameraInput &input, StereoEyeEnum eye, StereoModeEnum stereoMode = StereoModeEnum::Separate);
}

#endif // guard_camera_h_m1nv5e8967gsx856dghzgg
