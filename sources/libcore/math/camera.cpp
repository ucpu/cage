#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/camera.h>

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
		CAGE_ASSERT(fov > rads(0), real(fov).value, aspectRatio.value, near.value, far.value);
		CAGE_ASSERT(aspectRatio != 0, real(fov).value, aspectRatio.value, near.value, far.value);
		CAGE_ASSERT(sign(near) == sign(far) && near != far, real(fov).value, aspectRatio.value, near.value, far.value);
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
		CAGE_ASSERT(left != right, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT(bottom != top, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT(sign(near) == sign(far) && near != far, left.value, right.value, bottom.value, top.value, near.value, far.value);
		return mat4(
			near * 2.0 / (right - left), 0, 0, 0,
			0, near * 2.0 / (top - bottom), 0, 0,
			(right + left) / (right - left), (top + bottom) / (top - bottom), -(far + near) / (far - near), -1,
			0, 0, -2 * far * near / (far - near), 0
		);
	}

	mat4 orthographicProjection(real left, real right, real bottom, real top, real near, real far)
	{
		CAGE_ASSERT(left != right, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT(bottom != top, left.value, right.value, bottom.value, top.value, near.value, far.value);
		CAGE_ASSERT(near != far, left.value, right.value, bottom.value, top.value, near.value, far.value);
		return transpose(mat4(
			2 / (right - left), 0, 0, -(right + left) / (right - left),
			0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
			0, 0, -2 / (far - near), -(far + near) / (far - near),
			0, 0, 0, 1
		));
	}




	bool frustumCulling(const vec3 &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplemented, "frustumCulling");
	}

	bool frustumCulling(const line &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplemented, "frustumCulling");
	}

	bool frustumCulling(const triangle &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplemented, "frustumCulling");
	}

	bool frustumCulling(const plane &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplemented, "frustumCulling");
	}

	bool frustumCulling(const sphere &shape, const mat4 &mvp)
	{
		CAGE_THROW_CRITICAL(notImplemented, "frustumCulling");
	}

	namespace
	{
		vec4 column(const mat4 &m, uint32 index)
		{
			return vec4(m[index], m[index + 4], m[index + 8], m[index + 12]);
		}
	}

	bool frustumCulling(const aabb &box, const mat4 &mvp)
	{
		vec4 planes[6] = {
			column(mvp, 3) + column(mvp, 0),
			column(mvp, 3) - column(mvp, 0),
			column(mvp, 3) + column(mvp, 1),
			column(mvp, 3) - column(mvp, 1),
			column(mvp, 3) + column(mvp, 2),
			column(mvp, 3) - column(mvp, 2),
		};
		const vec3 b[] = { box.a, box.b };
		for (uint32 i = 0; i < 6; i++)
		{
			const vec4 &p = planes[i]; // current plane
			const vec3 pv = vec3( // current p-vertex
				b[!!(p[0] > 0)][0],
				b[!!(p[1] > 0)][1],
				b[!!(p[2] > 0)][2]
			);
			const real d = dot(vec3(p), pv);
			if (d < -p[3])
				return false;
		}
		return true;
	}





	stereoModeEnum stringToStereoMode(const string & mode)
	{
		string m = mode.toLower();
		if (m == "mono") return stereoModeEnum::Mono;
		if (m == "horizontal") return stereoModeEnum::Horizontal;
		if (m == "vertical") return stereoModeEnum::Vertical;
		CAGE_THROW_ERROR(exception, "invalid stereo mode name");
	}

	string stereoModeToString(stereoModeEnum mode)
	{
		switch (mode)
		{
		case stereoModeEnum::Mono: return "mono";
		case stereoModeEnum::Horizontal: return "horizontal";
		case stereoModeEnum::Vertical: return "vertical";
		default: CAGE_THROW_CRITICAL(exception, "invalid stereo mode enum");
		}
	}

	stereoCameraInput::stereoCameraInput() : viewportSize(1), orthographic(false)
	{}

	stereoCameraOutput stereoCamera(const stereoCameraInput &input, stereoModeEnum stereoMode, stereoEyeEnum eye)
	{
		real viewportX = input.viewportOrigin[0];
		real viewportY = input.viewportOrigin[1];
		real viewportWidth = input.viewportSize[0];
		real viewportHeight = input.viewportSize[1];
		real aspectRatio = input.aspectRatio * viewportWidth / viewportHeight;
		switch (eye)
		{
		case stereoEyeEnum::Mono:
			break;
		case stereoEyeEnum::Left:
			switch (stereoMode)
			{
			case stereoModeEnum::Mono:
				break;
			case stereoModeEnum::Horizontal:
				viewportWidth *= 0.5;
				viewportX *= 0.5;
				break;
			case stereoModeEnum::Vertical:
				viewportHeight *= 0.5;
				viewportY *= 0.5;
				viewportY += 0.5;
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid stereo mode");
			}
			break;
		case stereoEyeEnum::Right:
			switch (stereoMode)
			{
			case stereoModeEnum::Mono:
				break;
			case stereoModeEnum::Horizontal:
				viewportWidth *= 0.5;
				viewportX *= 0.5;
				viewportX += 0.5;
				break;
			case stereoModeEnum::Vertical:
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
		stereoCameraOutput out;
		vec3 p = input.position;
		vec3 forward = input.orientation * vec3(0, 0, -1);
		vec3 up = input.orientation * vec3(0, 1, 0);
		switch (eye)
		{
		case stereoEyeEnum::Mono:
			if (input.orthographic)
				out.projection = orthographicProjection(-1, 1, -1, 1, input.near, input.far);
			else
				out.projection = perspectiveProjection(input.fov, aspectRatio, input.near, input.far);
			break;
		case stereoEyeEnum::Left:
			if (input.orthographic)
				out.projection = orthographicProjection(-1, 1, -1, 1, input.near, input.far);
			else
				out.projection = perspectiveProjection(input.fov, aspectRatio, input.near, input.far, input.zeroParallaxDistance, -input.eyeSeparation);
			p -= cross(forward, up) * (input.eyeSeparation * 0.5);
			break;
		case stereoEyeEnum::Right:
			if (input.orthographic)
				out.projection = orthographicProjection(-1, 1, -1, 1, input.near, input.far);
			else
				out.projection = perspectiveProjection(input.fov, aspectRatio, input.near, input.far, input.zeroParallaxDistance, input.eyeSeparation);
			p += cross(forward, up) * (input.eyeSeparation * 0.5);
			break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid eye");
		}
		out.view = mat4(inverse(transform(p, input.orientation, input.scale)));
		out.viewportOrigin = vec2(viewportX, viewportY);
		out.viewportSize = vec2(viewportWidth, viewportHeight);
		return out;
	}
}
