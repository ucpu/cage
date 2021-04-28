#include <cage-core/geometry.h>
#include <cage-core/camera.h>
#include <cage-core/string.h>

namespace cage
{
	mat4 lookAt(const vec3 &eye, const vec3 &target, const vec3 &up)
	{
		CAGE_ASSERT(eye != target);
		CAGE_ASSERT(up != vec3());
		vec3 f = normalize(target - eye);
		vec3 u = normalize(up);
		vec3 s = normalize(cross(f, u));
		u = cross(s, f);
		mat4 res;
		res[0] = s[0];
		res[4] = s[1];
		res[8] = s[2];
		res[1] = u[0];
		res[5] = u[1];
		res[9] = u[2];
		res[2] = -f[0];
		res[6] = -f[1];
		res[10] = -f[2];
		res[12] = -dot(s, eye);
		res[13] = -dot(u, eye);
		res[14] = dot(f, eye);
		return res;
	}

	mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far)
	{
		CAGE_ASSERT(fov > rads(0));
		CAGE_ASSERT(aspectRatio > 0);
		CAGE_ASSERT(sign(near) == sign(far) && near != far);
		real f = 1 / tan(fov / 2);
		return mat4(
			f / aspectRatio, 0, 0, 0,
			0, f, 0, 0,
			0, 0, (far + near) / (near - far), -1,
			0, 0, far * near * 2.0 / (near - far), 0
		);
	}

	mat4 perspectiveProjection(rads fov, real aspectRatio, real near, real far, real zeroParallaxDistance, real eyeSeparation)
	{
		CAGE_ASSERT(fov > rads(0));
		CAGE_ASSERT(aspectRatio > 0);
		CAGE_ASSERT(sign(near) == sign(far) && near != far);
		real baseLength = near * tan(fov * 0.5);
		real stereoOffset = 0.5 * eyeSeparation * near / zeroParallaxDistance;
		real left = -aspectRatio * baseLength + stereoOffset;
		real right = aspectRatio * baseLength + stereoOffset;
		real top = baseLength;
		real bottom = -baseLength;
		return perspectiveProjection(left, right, bottom, top, near, far);
	}

	mat4 perspectiveProjection(real left, real right, real bottom, real top, real near, real far)
	{
		CAGE_ASSERT(left != right);
		CAGE_ASSERT(bottom != top);
		CAGE_ASSERT(sign(near) == sign(far) && near != far);
		return mat4(
			near * 2.0 / (right - left), 0, 0, 0,
			0, near * 2.0 / (top - bottom), 0, 0,
			(right + left) / (right - left), (top + bottom) / (top - bottom), (far + near) / (near - far), -1,
			0, 0, 2 * far * near / (near - far), 0
		);
	}

	mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far)
	{
		CAGE_ASSERT(left != right);
		CAGE_ASSERT(bottom != top);
		CAGE_ASSERT(near != far);
		return transpose(mat4(
			2 / (right - left), 0, 0, -(right + left) / (right - left),
			0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
			0, 0, -2 / (far - near), -(far + near) / (far - near),
			0, 0, 0, 1
		));
	}

	StereoModeEnum stringToStereoMode(const string & mode)
	{
		const string m = toLower(mode);
		if (m == "mono") return StereoModeEnum::Mono;
		if (m == "horizontal") return StereoModeEnum::Horizontal;
		if (m == "vertical") return StereoModeEnum::Vertical;
		CAGE_THROW_ERROR(Exception, "invalid stereo mode name");
	}

	string stereoModeToString(StereoModeEnum mode)
	{
		switch (mode)
		{
		case StereoModeEnum::Mono: return "mono";
		case StereoModeEnum::Horizontal: return "horizontal";
		case StereoModeEnum::Vertical: return "vertical";
		default: CAGE_THROW_CRITICAL(Exception, "invalid stereo mode enum");
		}
	}

	StereoCameraOutput stereoCamera(const StereoCameraInput &input, StereoModeEnum stereoMode, StereoEyeEnum eye)
	{
		real viewportX = input.viewportOrigin[0];
		real viewportY = input.viewportOrigin[1];
		real viewportWidth = input.viewportSize[0];
		real viewportHeight = input.viewportSize[1];
		real aspectRatio = input.aspectRatio * viewportWidth / viewportHeight;
		switch (eye)
		{
		case StereoEyeEnum::Mono:
			break;
		case StereoEyeEnum::Left:
			switch (stereoMode)
			{
			case StereoModeEnum::Mono:
				break;
			case StereoModeEnum::Horizontal:
				viewportWidth *= 0.5;
				viewportX *= 0.5;
				break;
			case StereoModeEnum::Vertical:
				viewportHeight *= 0.5;
				viewportY *= 0.5;
				viewportY += 0.5;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid stereo mode");
			}
			break;
		case StereoEyeEnum::Right:
			switch (stereoMode)
			{
			case StereoModeEnum::Mono:
				break;
			case StereoModeEnum::Horizontal:
				viewportWidth *= 0.5;
				viewportX *= 0.5;
				viewportX += 0.5;
				break;
			case StereoModeEnum::Vertical:
				viewportHeight *= 0.5;
				viewportY *= 0.5;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid stereo mode");
			}
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid eye");
		}
		StereoCameraOutput out;
		vec3 p = input.position;
		vec3 forward = input.orientation * vec3(0, 0, -1);
		vec3 up = input.orientation * vec3(0, 1, 0);
		switch (eye)
		{
		case StereoEyeEnum::Mono:
			if (input.orthographic)
				out.projection = orthographicProjection(-1, 1, -1, 1, input.near, input.far);
			else
				out.projection = perspectiveProjection(input.fov, aspectRatio, input.near, input.far);
			break;
		case StereoEyeEnum::Left:
			if (input.orthographic)
				out.projection = orthographicProjection(-1, 1, -1, 1, input.near, input.far);
			else
				out.projection = perspectiveProjection(input.fov, aspectRatio, input.near, input.far, input.zeroParallaxDistance, -input.eyeSeparation);
			p -= cross(forward, up) * (input.eyeSeparation * 0.5);
			break;
		case StereoEyeEnum::Right:
			if (input.orthographic)
				out.projection = orthographicProjection(-1, 1, -1, 1, input.near, input.far);
			else
				out.projection = perspectiveProjection(input.fov, aspectRatio, input.near, input.far, input.zeroParallaxDistance, input.eyeSeparation);
			p += cross(forward, up) * (input.eyeSeparation * 0.5);
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid eye");
		}
		out.view = mat4(inverse(transform(p, input.orientation, input.scale)));
		out.viewportOrigin = vec2(viewportX, viewportY);
		out.viewportSize = vec2(viewportWidth, viewportHeight);
		return out;
	}
}
