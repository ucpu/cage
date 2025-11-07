#ifndef guard_cameraRay_as6erz4gt891edf
#define guard_cameraRay_as6erz4gt891edf

#include <cage-core/geometry.h>
#include <cage-engine/core.h>

namespace cage
{
	class Entity;

	Line cameraRay(Entity *camera, Vec2 screenPosition);
	Line cameraMouseRay(Entity *camera);
	Line cameraCenterRay(Entity *camera);
}

#endif // guard_cameraRay_as6erz4gt891edf
