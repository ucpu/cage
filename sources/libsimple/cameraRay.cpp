#include <cage-simple/cameraRay.h>
#include <cage-simple/engine.h>
#include <cage-engine/window.h>
#include <cage-engine/scene.h>
#include <cage-core/entities.h>
#include <cage-core/camera.h>

namespace cage
{
	Line cameraRay(Entity *camera, Vec2 screenPosition)
	{
		CAGE_ASSERT(camera);
		CAGE_ASSERT(camera->has<TransformComponent>());
		CAGE_ASSERT(camera->has<CameraComponent>());
		CAGE_ASSERT(screenPosition.valid());
		const Vec2i res = engineWindow()->resolution();
		Vec2 p = screenPosition;
		p /= Vec2(res[0], res[1]);
		p = p * 2 - 1;
		Real px = p[0], py = -p[1];
		const TransformComponent &ts = camera->value<TransformComponent>();
		const CameraComponent &cs = camera->value<CameraComponent>();
		CAGE_ASSERT(cs.cameraType == CameraTypeEnum::Perspective);
		const Mat4 view = inverse(Mat4(ts.position, ts.orientation, Vec3(ts.scale)));
		const Mat4 proj = perspectiveProjection(cs.camera.perspectiveFov, Real(res[0]) / Real(res[1]), cs.near, cs.far);
		const Mat4 inv = inverse(proj * view);
		const Vec4 pn = inv * Vec4(px, py, -1, 1);
		const Vec4 pf = inv * Vec4(px, py, 1, 1);
		const Vec3 near = Vec3(pn) / pn[3];
		const Vec3 far = Vec3(pf) / pf[3];
		return makeSegment(near, far);
	}

	Line cameraMouseRay(Entity *camera)
	{
		return cameraRay(camera, engineWindow()->mousePosition());
	}

	Line cameraCenterRay(Entity *camera)
	{
		const Vec2i res = engineWindow()->resolution();
		return cameraRay(camera, Vec2(res) * 0.5);
	}
}
