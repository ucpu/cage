#include <cage-core/core.h>
#include <cage-core/math.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>

namespace cage
{
	stereoModeEnum stringToStereoMode(const string & mode)
	{
		string m = mode.toLower();
		if (m == "mono") return stereoModeEnum::Mono;
		if (m == "horizontal") return stereoModeEnum::LeftRight;
		if (m == "vertical") return stereoModeEnum::TopBottom;
		CAGE_THROW_ERROR(exception, "invalid window mode name");
	}

	string stereoModeToString(stereoModeEnum mode)
	{
		switch (mode)
		{
		case stereoModeEnum::Mono: return "mono";
		case stereoModeEnum::LeftRight: return "horizontal";
		case stereoModeEnum::TopBottom: return "vertical";
		default: CAGE_THROW_CRITICAL(exception, "invalid window mode");
		}
	}

	stereoCameraStruct::stereoCameraStruct() : ortographic(false) {}

	void stereoscopy(eyeEnum eye, const stereoCameraStruct & camera, real aspectRatio, stereoModeEnum stereoMode, mat4 & view, mat4 & projection, real & viewportX, real & viewportY, real & viewportWidth, real & viewportHeight)
	{
		aspectRatio *= viewportWidth / viewportHeight;
		switch (eye)
		{
		case eyeEnum::Mono:
			break;
		case eyeEnum::Left:
			switch (stereoMode)
			{
			case stereoModeEnum::Mono:
				break;
			case stereoModeEnum::LeftRight:
				viewportWidth *= 0.5;
				viewportX *= 0.5;
				break;
			case stereoModeEnum::TopBottom:
				viewportHeight *= 0.5;
				viewportY *= 0.5;
				viewportY += 0.5;
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid stereo mode");
			}
			break;
		case eyeEnum::Right:
			switch (stereoMode)
			{
			case stereoModeEnum::Mono:
				break;
			case stereoModeEnum::LeftRight:
				viewportWidth *= 0.5;
				viewportX *= 0.5;
				viewportX += 0.5;
				break;
			case stereoModeEnum::TopBottom:
				viewportHeight *= 0.5;
				viewportY *= 0.5;
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid stereo mode");
			}
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid eye");
		}
		vec3 p = camera.position;
		switch (eye)
		{
		case eyeEnum::Mono:
			if (camera.ortographic)
				projection = orthographicProjection(-1, 1, -1, 1, camera.near, camera.far);
			else
				projection = perspectiveProjection(camera.fov, aspectRatio, camera.near, camera.far);
			break;
		case eyeEnum::Left:
			if (camera.ortographic)
				projection = orthographicProjection(-1, 1, -1, 1, camera.near, camera.far);
			else
				projection = perspectiveProjection(camera.fov, aspectRatio, camera.near, camera.far, camera.zeroParallaxDistance, -camera.eyeSeparation);
			p -= cross(camera.direction, camera.worldUp) * (camera.eyeSeparation * 0.5);
			break;
		case eyeEnum::Right:
			if (camera.ortographic)
				projection = orthographicProjection(-1, 1, -1, 1, camera.near, camera.far);
			else
				projection = perspectiveProjection(camera.fov, aspectRatio, camera.near, camera.far, camera.zeroParallaxDistance, camera.eyeSeparation);
			p += cross(camera.direction, camera.worldUp) * (camera.eyeSeparation * 0.5);
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid eye");
		}
		view = lookAt(p, p + camera.direction, camera.worldUp);
	}
}