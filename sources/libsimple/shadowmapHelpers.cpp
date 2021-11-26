#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scene.h>

#include <cage-simple/engine.h>
#include <cage-simple/shadowmapHelpers.h>

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
		TransformComponent &t = e->value<TransformComponent>();
		RenderComponent &r = e->value<RenderComponent>();
		Aabb b = getBoxForAsset(r.object);
		return b * t;
	}

	Aabb getBoxForScene(uint32 sceneMask)
	{
		Aabb res;
		for (Entity *e : engineEntities()->component<RenderComponent>()->entities())
		{
			RenderComponent &r = e->value<RenderComponent>();
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
			LightComponent &l = light->value<LightComponent>();
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
		TransformComponent &t = light->value<TransformComponent>();
		ShadowmapComponent &s = light->value<ShadowmapComponent>();
		if (box.empty())
			s.worldSize = Vec3(1);
		else
		{
			t.position = box.center();
			s.worldSize = Vec3(box.diagonal() * 0.5);
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
		CameraComponent &c = camera->value<CameraComponent>();
		// todo utilize the camera frustum to make the shadowmap tighter
		return fitShadowmapForDirectionalLight(light, c.sceneMask);
	}
}
