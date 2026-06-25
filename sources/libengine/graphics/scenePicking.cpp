#include <algorithm>

#include <cage-core/assetsManager.h>
#include <cage-core/collider.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/hashString.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/spatialStructure.h>
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
			Entity *e = nullptr;
			uint32 id = 0;
			uint32 chunkIndex = m;
		};

		struct Chunk
		{
			Aabb box;
			Holder<RenderObject> object;
			std::vector<std::vector<Holder<Model>>> objectsModels;
			PointerRange<Candidate> candidates;
		};

		class ScenePickingImpl : public ScenePicking
		{
		public:
			ScenePickingConfig config;

			std::vector<Candidate> candidates;
			std::vector<Chunk> chunks;
			Holder<SpatialStructure> ss;
			Holder<SpatialQuery> sq;

			ScenePickingImpl(const ScenePickingConfig &config) : config(config)
			{
				CAGE_ASSERT(config.lodSelection.center.valid());
				CAGE_ASSERT(config.lodSelection.screenSize > 0);
				CAGE_ASSERT(config.assets);
				CAGE_ASSERT(config.entities);

				candidates.reserve(config.entities->component<PickableComponent>()->count());
				chunks.reserve(100);

				// models
				entitiesVisitor(
					[&](Entity *e, const TransformComponent &tr, const ModelComponent &md, const PickableComponent &)
					{
						if (md.modelId != 0)
							candidates.push_back({ tr, e, md.modelId });
					},
					config.entities, false);

				// icons
				entitiesVisitor(
					[&](Entity *e, const TransformComponent &tr, const SpriteComponent &ic, const PickableComponent &)
					{
						static constexpr uint32 DefaultIcon = HashString("cage/models/icon.obj");
						candidates.push_back({ tr, e, ic.modelId ? ic.modelId : DefaultIcon });
					},
					config.entities, false);

				// prepare chunk
				const auto &pushMesh = [&](const auto b, const auto e, Holder<Model> &&md)
				{
					if (!md->collider)
						return;
					Chunk ch;
					ch.box = md->boundingBox;
					ch.objectsModels.resize(1);
					ch.objectsModels[0].push_back(std::move(md));
					ch.candidates = { &*b, &*b + (e - b) };
					chunks.push_back(std::move(ch));
				};

				const auto &pushChunk = [&](const auto b, const auto e)
				{
					if (Holder<Model> md = config.assets->get<Model>(b->id))
					{
						pushMesh(b, e, std::move(md));
						return;
					}

					if (Holder<RenderObject> o = config.assets->get<RenderObject>(b->id))
					{
						if (o->lodsCount() == 1 && o->models(0).size() == 1)
						{
							if (Holder<Model> md = config.assets->get<Model>(o->models(0)[0]))
								pushMesh(b, e, std::move(md));
							return;
						}

						Chunk ch;
						ch.objectsModels.resize(o->lodsCount());
						for (uint32 lod = 0; lod < o->lodsCount(); lod++)
						{
							for (uint32 i : o->models(lod))
							{
								if (Holder<Model> md = config.assets->get<Model>(i))
								{
									if (!md->collider)
										continue;
									ch.box += md->boundingBox;
									ch.objectsModels[lod].push_back(std::move(md));
								}
							}
						}
						ch.object = std::move(o);
						ch.candidates = { &*b, &*b + (e - b) };
						chunks.push_back(std::move(ch));
					}
				};

				// chunking
				std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) { return a.id < b.id; });
				auto first = candidates.begin();
				while (first != candidates.end())
				{
					auto last = std::find_if(first, candidates.end(), [&](const Candidate &c) { return c.id != first->id; });
					pushChunk(first, last);
					first = last;
				}

				// spatial
				if (config.allowSpatialStructure && candidates.size() > 1'000)
				{
					ss = newSpatialStructure({ .reserve = numeric_cast<uint32>(candidates.size()) });
					for (const Chunk &ch : chunks)
					{
						for (Candidate &c : ch.candidates)
						{
							c.chunkIndex = &ch - chunks.data();
							ss->update(&c - candidates.data(), ch.box * c.tr);
						}
					}
					ss->rebuild();
					sq = newSpatialQuery(ss.share());
				}
			}

			Holder<PointerRange<ScenePickingResult>> pick(Line picker) const
			{
				CAGE_ASSERT(picker.valid());

				PointerRangeHolder<ScenePickingResult> results;
				results.reserve(100);

				const auto &check = [&](const Chunk &ch, const Candidate &c)
				{
					const uint32 lod = ch.object ? config.lodSelection.selectLod(c.tr.position, +ch.object) : 0;
					for (const auto &md : ch.objectsModels[lod])
					{
						if (!intersects(md->boundingBox * c.tr, picker))
							continue;
						ScenePickingResult r;
						r.entity = c.e;
						r.point = intersection(picker, +md->collider, c.tr);
						if (r.point.valid())
							results.push_back(r);
					}
				};

				if (ss)
				{
					if (sq->intersection(picker))
					{
						for (uint32 ci : sq->result())
						{
							const Candidate &c = candidates[ci];
							const Chunk &ch = chunks[c.chunkIndex];
							check(ch, c);
						}
					}
				}
				else
				{
					for (const Chunk &ch : chunks)
					{
						for (const Candidate &c : ch.candidates)
						{
							if (!intersects(ch.box * c.tr, picker))
								continue;
							check(ch, c);
						}
					}
				}

				// finish results
				if (!results.empty())
					std::sort(results.begin(), results.end(), [&](const ScenePickingResult &a, const ScenePickingResult &b) -> bool { return distanceSquared(a.point, picker.a()) < distanceSquared(b.point, picker.a()); });
				return results;
			}
		};
	}

	Holder<PointerRange<ScenePickingResult>> ScenePicking::pick(Line picker) const
	{
		const ScenePickingImpl *impl = (const ScenePickingImpl *)this;
		return impl->pick(picker);
	}

	Holder<ScenePicking> newScenePicking(const ScenePickingConfig &config)
	{
		return systemMemory().createImpl<ScenePicking, ScenePickingImpl>(config);
	}
}
