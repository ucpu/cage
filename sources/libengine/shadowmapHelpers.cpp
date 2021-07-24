#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>

#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/engine.h>
#include <cage-engine/shadowmapHelpers.h>

namespace cage
{
	Aabb getBoxForModel(uint32 name)
	{
		Holder<Model> m = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(name);
		if (m)
			return m->boundingBox();
		return Aabb();
	}

	Aabb getBoxForObject(uint32 name)
	{
		Holder<RenderObject> o = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
		if (!o)
			return Aabb();
		Aabb res;
		for (uint32 it : o->models(0))
			res += getBoxForModel(it);
		return res;
	}

	Aabb getBoxForAsset(uint32 name)
	{
		AssetManager *ass = engineAssets();
		{
			Holder<Model> m = ass->tryGet<AssetSchemeIndexModel, Model>(name);
			if (m)
				return m->boundingBox();
		}
		{
			Holder<RenderObject> o = ass->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
			if (o)
				return getBoxForObject(name);
		}
		return Aabb();
	}

	Aabb getBoxForEntity(Entity *e)
	{
		CAGE_ASSERT(e->has<TransformComponent>());
		CAGE_ASSERT(e->has<RenderComponent>());
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		CAGE_COMPONENT_ENGINE(Render, r, e);
		Aabb b = getBoxForAsset(r.object);
		return b * t;
	}

	Aabb getBoxForScene(uint32 sceneMask)
	{
		Aabb res;
		for (Entity *e : engineEntities()->component<RenderComponent>()->entities())
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
			if (!light->has<TransformComponent>() || !light->has<LightComponent>() || !light->has<ShadowmapComponent>())
				return false;
			CAGE_COMPONENT_ENGINE(Light, l, light);
			return l.lightType == LightTypeEnum::Directional;
		}

		bool isEntityCamera(Entity *camera)
		{
			return camera->has<TransformComponent>() && camera->has<CameraComponent>();
		}
	}

	void fitShadowmapForDirectionalLight(Entity *light, const Aabb &box)
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
