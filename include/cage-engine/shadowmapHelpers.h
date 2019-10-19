#ifndef guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
#define guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01

namespace cage
{
	CAGE_API aabb getBoxForRenderMesh(uint32 name);
	CAGE_API aabb getBoxForRenderObject(uint32 name);
	CAGE_API aabb getBoxForAsset(uint32 name);

	CAGE_API aabb getBoxForRenderEntity(entity *e);
	CAGE_API aabb getBoxForCameraEntity(entity *e);
	CAGE_API aabb getBoxForRenderScene(uint32 sceneMask = 1);

	CAGE_API void fitShadowmapForDirectionalLight(entity *light, const aabb &scene);
	CAGE_API void fitShadowmapForDirectionalLight(entity *light, const aabb &scene, entity *camera);
	CAGE_API void fitShadowmapForDirectionalLight(entity *light, entity *camera);
	CAGE_API void fitShadowmapForDirectionalLight(entity *light);
}

#endif // guard_shadowmapHelpers_h_FDF074217A66481BBFE8DFD4680D9E01
