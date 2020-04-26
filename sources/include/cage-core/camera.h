#ifndef guard_projections_h_m1nv5e8967gsx856dghzgg
#define guard_projections_h_m1nv5e8967gsx856dghzgg

#include "math.h"

namespace cage
{
	CAGE_CORE_API mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up);

	// fov is along vertical axis
	CAGE_CORE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far);
	CAGE_CORE_API mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far, real zeroParallaxDistance, real eyeSeparation); // use negative eyeSeparation for left eye
	CAGE_CORE_API mat4 perspectiveProjection(real left, real right, real bottom, real top, real near, real far);
	CAGE_CORE_API mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far);

	CAGE_CORE_API bool frustumCulling(const vec3 &shape, const mat4 &mvp);
	CAGE_CORE_API bool frustumCulling(const line &shape, const mat4 &mvp);
	CAGE_CORE_API bool frustumCulling(const triangle &shape, const mat4 &mvp);
	CAGE_CORE_API bool frustumCulling(const plane &shape, const mat4 &mvp);
	CAGE_CORE_API bool frustumCulling(const sphere &shape, const mat4 &mvp);
	CAGE_CORE_API bool frustumCulling(const aabb &shape, const mat4 &mvp);

	enum class StereoModeEnum : uint32
	{
		Mono,
		Horizontal,
		Vertical,
		// todo separate buffers
	};

	enum class StereoEyeEnum : uint32
	{
		Mono,
		Left,
		Right,
	};

	struct CAGE_CORE_API StereoCameraInput : public transform
	{
		vec2 viewportOrigin; // 0 .. 1
		vec2 viewportSize = vec2(1); // 0 .. 1
		real aspectRatio;
		rads fov;
		real near;
		real far;
		real zeroParallaxDistance;
		real eyeSeparation;
		bool orthographic = false;
	};

	struct CAGE_CORE_API StereoCameraOutput
	{
		mat4 view;
		mat4 projection;
		vec2 viewportOrigin; // 0 .. 1
		vec2 viewportSize; // 0 .. 1
	};

	CAGE_CORE_API StereoModeEnum stringToStereoMode(const string &mode);
	CAGE_CORE_API string stereoModeToString(StereoModeEnum mode);
	CAGE_CORE_API StereoCameraOutput stereoCamera(const StereoCameraInput &input, StereoModeEnum stereoMode, StereoEyeEnum eye);
}

#endif // guard_projections_h_m1nv5e8967gsx856dghzgg
