#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>

#include <cage-engine/graphics.h>
#include <cage-engine/engine.h>
#include <cage-engine/shadowmapHelpers.h>

namespace cage
{
	aabb getBoxForRenderMesh(uint32 name)
	{
		Holder<Mesh> m = engineAssets()->tryGet<AssetSchemeIndexMesh, Mesh>(name);
		if (m)
			return m->getBoundingBox();
		return aabb();
	}

	aabb getBoxForRenderObject(uint32 name)
	{
		Holder<RenderObject> o = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
		if (!o)
			return aabb();
		aabb res;
		for (uint32 it : o->meshes(0))
			res += getBoxForRenderMesh(it);
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
				return getBoxForRenderObject(name);
		}
		return aabb();
	}

	aabb getBoxForRenderEntity(Entity *e)
	{
		CAGE_ASSERT(e->has(TransformComponent::component));
		CAGE_ASSERT(e->has(RenderComponent::component));
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		CAGE_COMPONENT_ENGINE(Render, r, e);
		aabb b = getBoxForAsset(r.object);
		return b * t;
	}

	aabb getBoxForCameraEntity(Entity *e)
	{
		CAGE_ASSERT(e->has(TransformComponent::component));
		CAGE_ASSERT(e->has(CameraComponent::component));
		CAGE_THROW_CRITICAL(NotImplemented, "getBoxForCameraEntity");
	}

	aabb getBoxForRenderScene(uint32 sceneMask)
	{
		aabb res;
		for (Entity *e : RenderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Render, r, e);
			if (r.sceneMask & sceneMask)
				res += getBoxForRenderEntity(e);
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
	}

	void fitShadowmapForDirectionalLight(Entity *light, const aabb &scene)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(Transform, t, light);
		CAGE_COMPONENT_ENGINE(Shadowmap, s, light);
		if (scene.empty())
			s.worldSize = vec3(1);
		else
		{
			t.position = scene.center();
			s.worldSize = vec3(scene.diagonal() * 0.5);
		}
	}

	void fitShadowmapForDirectionalLight(Entity *light, const aabb &scene, Entity *camera)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_THROW_CRITICAL(NotImplemented, "fitShadowmapForDirectionalLight with camera");
	}

	void fitShadowmapForDirectionalLight(Entity *light, Entity *camera)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(Shadowmap, s, light);
		fitShadowmapForDirectionalLight(light, getBoxForRenderScene(s.sceneMask), camera);
	}

	void fitShadowmapForDirectionalLight(Entity *light)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(light));
		CAGE_COMPONENT_ENGINE(Shadowmap, s, light);
		fitShadowmapForDirectionalLight(light, getBoxForRenderScene(s.sceneMask));
	}
}
