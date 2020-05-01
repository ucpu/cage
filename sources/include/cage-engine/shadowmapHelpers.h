#ifndef guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
#define guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01

#include "core.h"

namespace cage
{
	CAGE_ENGINE_API aabb getBoxForMesh(uint32 name);
	CAGE_ENGINE_API aabb getBoxForObject(uint32 name);
	CAGE_ENGINE_API aabb getBoxForAsset(uint32 name);
	CAGE_ENGINE_API aabb getBoxForEntity(Entity *e);
	CAGE_ENGINE_API aabb getBoxForScene(uint32 sceneMask);

	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, const aabb &box);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, uint32 sceneMask);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, Entity *camera);
}

#endif // guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
