#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>

#include <cage-engine/graphics.h>
#include <cage-engine/engine.h>
#include <cage-engine/shadowmapHelpers.h>

namespace cage
{
	aabb getBoxForMesh(uint32 name)
	{
		Holder<Mesh> m = engineAssets()->tryGet<AssetSchemeIndexMesh, Mesh>(name);
		if (m)
			return m->getBoundingBox();
		return aabb();
	}

	aabb getBoxForObject(uint32 name)
	{
		Holder<RenderObject> o = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
		if (!o)
			return aabb();
		aabb res;
		for (uint32 it : o->meshes(0))
			res += getBoxForMesh(it);
		return res;
	}

	aabb getBoxForAsset(uint32 name)
	{
		AssetManager *ass = engineAssets();
		{
			Holder<Mesh> m = ass->tryGet<AssetSchemeIndexMesh, Mesh>(name);
			if (m)
				return m->getBoundingBox();
		}
		{
			Holder<RenderObject> o = ass->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
			if (o)
				return getBoxForObject(name);
		}
		return aabb();
	}

	aabb getBoxForEntity(Entity *e)
	{
		CAGE_ASSERT(e->has(TransformComponent::component));
		CAGE_ASSERT(e->has(RenderComponent::component));
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		CAGE_COMPONENT_ENGINE(Render, r, e);
		aabb b = getBoxForAsset(r.object);
		return b * t;
	}

	aabb getBoxForScene(uint32 sceneMask)
	{
		aabb res;
		for (Entity *e : RenderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Render, r, e);
			if (r.sceneMask & sceneMask)
				res += getBoxForEntity(e);
		}
		return res;
	}

	namespace
	{
		bool isEntityDirectionalLightWithShadowmap(Entity *light)
		{
			if (!light->has(TransformComponent::component) || !light->has(LightComponent::component) || !light->has(ShadowmapComponent::component))
				return false;
			CAGE_COMPONENT_ENGINE(Light, l, light);
			return l.lightType == LightTypeEnum::Directional;
		}

		bool isEntityCamera(Entity *camera)
		{
			return camera->has(TransformComponent::component) && camera->has(CameraComponent::component);
		}
	}

	void fitShadowmapForDirectionalLight(Entity *light, const aabb &box)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(Transform, t, light);
		CAGE_COMPONENT_ENGINE(Shadowmap, s, light);
		if (box.empty())
			s.worldSize = vec3(1);
		else
		{
			t.position = box.center();
			s.worldSize = vec3(box.diagonal() * 0.5);
		}
	}

	void fitShadowmapForDirectionalLight(Entity *light, uint32 sceneMask)
	{
		fitShadowmapForDirectionalLight(light, getBoxForScene(sceneMask));
	}

	void fitShadowmapForDirectionalLight(Entity *light, Entity *camera)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_ASSERT(isEntityCamera(camera));
		CAGE_COMPONENT_ENGINE(Camera, c, camera);
		// todo utilize the camera frustum to make the shadowmap tighter
		return fitShadowmapForDirectionalLight(light, c.sceneMask);
	}
}
