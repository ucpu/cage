#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
//#include <cage-core/camera.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/engine.h>
#include <cage-engine/shadowmapHelpers.h>

namespace cage
{
	aabb getBoxForRenderMesh(uint32 name)
	{
		renderMesh *m = assets()->tryGet<assetSchemeIndexMesh, renderMesh>(name);
		if (m)
			return m->getBoundingBox();
		return aabb();
	}

	aabb getBoxForRenderObject(uint32 name)
	{
		renderObject *o = assets()->tryGet<assetSchemeIndexRenderObject, renderObject>(name);
		if (!o)
			return aabb();
		aabb res;
		for (uint32 it : o->meshes(0))
			res += getBoxForRenderMesh(it);
		return res;
	}

	aabb getBoxForAsset(uint32 name)
	{
		switch (assets()->scheme(name))
		{
		case assetSchemeIndexMesh: return getBoxForRenderMesh(name);
		case assetSchemeIndexRenderObject: return getBoxForRenderObject(name);
		case m: return aabb();
		default:
			CAGE_THROW_ERROR(exception, "cannot get box for asset with this scheme");
		}
	}

	aabb getBoxForRenderEntity(entity *e)
	{
		CAGE_ASSERT(e->has(transformComponent::component));
		CAGE_ASSERT(e->has(renderComponent::component));
		CAGE_COMPONENT_ENGINE(transform, t, e);
		CAGE_COMPONENT_ENGINE(render, r, e);
		aabb b = getBoxForAsset(r.object);
		return b * t;
	}

	aabb getBoxForCameraEntity(entity *e)
	{
		CAGE_ASSERT(e->has(transformComponent::component));
		CAGE_ASSERT(e->has(cameraComponent::component));
		CAGE_THROW_CRITICAL(notImplemented, "getBoxForCameraEntity");
	}

	aabb getBoxForRenderScene(uint32 sceneMask)
	{
		aabb res;
		for (entity *e : renderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(render, r, e);
			if (r.sceneMask & sceneMask)
				res += getBoxForRenderEntity(e);
		}
		return res;
	}

	namespace
	{
		bool isEntityDirectionalLightWithShadowmap(entity *light)
		{
			if (!light->has(transformComponent::component) || !light->has(lightComponent::component) || !light->has(shadowmapComponent::component))
				return false;
			CAGE_COMPONENT_ENGINE(light, l, light);
			return l.lightType == lightTypeEnum::Directional;
		}
	}

	void fitShadowmapForDirectionalLight(entity *light, const aabb &scene)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(transform, t, light);
		CAGE_COMPONENT_ENGINE(shadowmap, s, light);
		if (scene.empty())
			s.worldSize = vec3(1);
		else
		{
			t.position = scene.center();
			s.worldSize = vec3(scene.diagonal() * 0.5);
		}
	}

	void fitShadowmapForDirectionalLight(entity *light, const aabb &scene, entity *camera)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_THROW_CRITICAL(notImplemented, "fitShadowmapForDirectionalLight with camera");
	}

	void fitShadowmapForDirectionalLight(entity *light, entity *camera)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(shadowmap, s, light);
		fitShadowmapForDirectionalLight(light, getBoxForRenderScene(s.sceneMask), camera);
	}

	void fitShadowmapForDirectionalLight(entity *light)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(shadowmap, s, light);
		fitShadowmapForDirectionalLight(light, getBoxForRenderScene(s.sceneMask));
	}
}
