#ifndef guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
#define guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01

#include <cage-engine/core.h>

namespace cage
{
	class Entity;

	Aabb getBoxForModel(uint32 name);
	Aabb getBoxForObject(uint32 name);
	Aabb getBoxForAsset(uint32 name);
	Aabb getBoxForEntity(Entity *e);
	Aabb getBoxForScene(uint32 sceneMask);

	void fitShadowmapForDirectionalLight(Entity *light, const Aabb &box);
	void fitShadowmapForDirectionalLight(Entity *light, uint32 sceneMask);
	void fitShadowmapForDirectionalLight(Entity *light, Entity *camera);
}

#endif // guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
