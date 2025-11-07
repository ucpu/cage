#include <algorithm>

#include <cage-core/assetsManager.h>
#include <cage-core/collider.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/hashString.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scene.h>
#include <cage-engine/scenePicking.h>

namespace cage
{
	namespace
	{
		struct Candidate
		{
			TransformComponent tr;
			Holder<Model> md;
			Entity *e = nullptr;
		};
	}

	Holder<PointerRange<ScenePickingResult>> scenePicking(const ScenePickingConfig &config)
	{
		CAGE_ASSERT(config.picker.valid());
		CAGE_ASSERT(config.lodSelection.center.valid());
		CAGE_ASSERT(config.lodSelection.screenSize > 0);
		CAGE_ASSERT(config.assets);
		CAGE_ASSERT(config.entities);

		std::vector<Candidate> candidates;
		std::vector<Holder<Model>> tmpLod;
		const auto &findCandidates = [&](Entity *e, const TransformComponent &tr, uint32 id)
		{
			if (id == 0)
				return;
			if (Holder<Model> md = config.assets->get<Model>(id))
			{
				if (intersects(md->boundingBox * tr, config.picker))
					candidates.push_back({ tr, std::move(md), e });
				return;
			}
			if (Holder<RenderObject> o = config.assets->get<RenderObject>(id))
			{
				config.lodSelection.selectModels(tmpLod, tr.position, +o, config.assets);
				for (auto &it : tmpLod)
					if (intersects(it->boundingBox * tr, config.picker))
						candidates.push_back({ tr, std::move(it), e });
			}
		};

		// models
		entitiesVisitor([&](Entity *e, const TransformComponent &tr, const ModelComponent &md, const PickableComponent &) { findCandidates(e, tr, md.model); }, config.entities, false);

		// icons
		static constexpr uint32 defaultIcon = HashString("cage/models/icon.obj");
		entitiesVisitor([&](Entity *e, const TransformComponent &tr, const IconComponent &ic, const PickableComponent &) { findCandidates(e, tr, ic.model ? ic.model : defaultIcon); }, config.entities, false);

		// test candidates
		PointerRangeHolder<ScenePickingResult> results;
		for (const auto &it : candidates)
		{
			if (!it.md->collider)
				continue;
			ScenePickingResult r;
			r.entity = it.e;
			r.point = intersection(config.picker, +it.md->collider, it.tr);
			if (!r.point.valid())
				continue;
			results.push_back(r);
		}

		// finish results
		if (!results.empty())
			std::sort(results.begin(), results.end(), [&](const ScenePickingResult &a, const ScenePickingResult &b) -> bool { return distanceSquared(a.point, config.picker.a()) < distanceSquared(b.point, config.picker.a()); });
		return results;
	}
}
