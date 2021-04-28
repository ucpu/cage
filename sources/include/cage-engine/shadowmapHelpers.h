#ifndef guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
#define guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01

#include "core.h"

namespace cage
{
	CAGE_ENGINE_API Aabb getBoxForModel(uint32 name);
	CAGE_ENGINE_API Aabb getBoxForObject(uint32 name);
	CAGE_ENGINE_API Aabb getBoxForAsset(uint32 name);
	CAGE_ENGINE_API Aabb getBoxForEntity(Entity *e);
	CAGE_ENGINE_API Aabb getBoxForScene(uint32 sceneMask);

	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, const Aabb &box);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, uint32 sceneMask);
	CAGE_ENGINE_API void fitShadowmapForDirectionalLight(Entity *light, Entity *camera);
}

#endif // guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
