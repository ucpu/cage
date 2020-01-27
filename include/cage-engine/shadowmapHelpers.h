#ifndef guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
#define guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01

#include "core.h"

namespace cage
{
	CAGE_ENGINE_API aabb getBoxForRenderMesh(uint32 name);
	CAGE_ENGINE_API aabb getBoxForRenderObject(uint32 name);
	CAGE_ENGINE_API aabb getBoxForAsset(uint32 name);

	CAGE_ENGINE_API aabb getBoxForRenderEntity(Entity *e);
	CAGE_ENGINE_API aabb getBoxForCameraEntity(Entity *e);
	CAGE_ENGINE_API aabb getBoxForRenderScene(uint32 sceneMask = 1);

	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, const aabb &scene);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, const aabb &scene, Entity *camera);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, Entity *camera);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light);
}

#endif // guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
