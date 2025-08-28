#include <algorithm>

#include <cage-core/assetsManager.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scenePicking.h>

namespace cage
{
	namespace
	{
		struct Boxes
		{
			const AssetsManager *assets = nullptr;

			Aabb model(uint32 name) const
			{
				Holder<Model> m = assets->get<AssetSchemeIndexModel, Model>(name);
				if (!m)
					return Aabb();
				return m->boundingBox;
			}

			Aabb object(const RenderObject *o) const
			{
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
				if (name == 0)
					return Aabb();
				{
					Holder<Model> m = assets->get<AssetSchemeIndexModel, Model>(name);
					if (m)
						return m->boundingBox;
				}
				{
					Holder<RenderObject> o = assets->get<AssetSchemeIndexRenderObject, RenderObject>(name);
					if (o)
						return object(+o);
				}
				return Aabb();
			}
		};
	}

	Holder<PointerRange<ScenePickingResult>> scenePicking(const ScenePickingConfig &config)
	{
		CAGE_ASSERT(config.picker.valid());
		CAGE_ASSERT(config.assets);
		CAGE_ASSERT(config.entities);
		CAGE_ASSERT(!config.camera || config.camera->manager() == config.entities);
		CAGE_ASSERT(!config.camera || config.screenHeight > 0);

		PointerRangeHolder<ScenePickingResult> results;

		entitiesVisitor(
			[&](Entity *e, const TransformComponent &tr, const ModelComponent &md, const PickableComponent &)
			{
				// todo respect different precisions
				// todo animations and alpha cut
				const Aabb box = Boxes{ config.assets }.asset(md.model) * tr;
				const Line ln = intersection(box, config.picker);
				if (!ln.valid())
					return;
				ScenePickingResult r;
				r.entity = e;
				r.point = ln.a();
				results.push_back(r);
			},
			config.entities, false);

		// todo entities with icons instead of models

		if (!results.empty())
			std::sort(results.begin(), results.end(), [&](const ScenePickingResult &a, const ScenePickingResult &b) -> bool { return distanceSquared(a.point, config.picker.a()) < distanceSquared(b.point, config.picker.b()); });
		return results;
	}
}
