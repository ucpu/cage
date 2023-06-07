#include <cage-core/assetManager.h>
#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/geometry.h>
#include <cage-core/meshIoCommon.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneShadowmapFitting.h>

namespace cage
{
	namespace
	{
		bool isEntityDirectionalLightWithShadowmap(Entity *light)
		{
			if (!light->has<TransformComponent>() || !light->has<LightComponent>() || !light->has<ShadowmapComponent>())
				return false;
			return light->value<LightComponent>().lightType == LightTypeEnum::Directional;
		}

		struct Boxes
		{
			AssetManager *assets = nullptr;

			Aabb model(uint32 name) const
			{
				Holder<Model> m = assets->tryGet<AssetSchemeIndexModel, Model>(name);
				if (m && any(m->flags & MeshRenderFlags::ShadowCast))
					return m->boundingBox();
				return Aabb();
			}

			Aabb object(uint32 name) const
			{
				Holder<RenderObject> o = assets->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
				if (!o)
					return Aabb();
				Aabb res;
				for (uint32 i = 0; i < o->lodsCount(); i++) // not sure which LOD is loaded, so we need to check all
					for (uint32 it : o->models(i))
						res += model(it);
				return res;
			}

			Aabb asset(uint32 name) const
			{
				{
					Holder<Model> m = assets->tryGet<AssetSchemeIndexModel, Model>(name);
					if (m)
						return m->boundingBox();
				}
				{
					Holder<RenderObject> o = assets->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
					if (o)
						return object(name);
				}
				return Aabb();
			}
		};
	}

	void shadowmapFitting(const ShadowmapFittingConfig &config)
	{
		CAGE_ASSERT(isEntityDirectionalLightWithShadowmap(config.light));

		uint32 mask = config.sceneMask;
		if (config.camera)
			mask &= config.camera->value<CameraComponent>().sceneMask;

		// todo use camera frustum for fitting the shadowmap

		Boxes boxes;
		boxes.assets = config.assets;
		Aabb box;
		entitiesVisitor(
			[&](Entity *e, const TransformComponent &t, const RenderComponent &r)
			{
				if (r.sceneMask & mask)
					box += boxes.asset(r.object) * t;
			},
			config.light->manager(), false);

		config.light->value<TransformComponent>().position = box.empty() ? Vec3() : box.center();
		config.light->value<ShadowmapComponent>().worldSize = Vec3(max(box.diagonal() * 0.5, 1e-3));
	}
}
