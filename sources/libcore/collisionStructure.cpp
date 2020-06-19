#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/collisionStructure.h>
#include <cage-core/collider.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/unordered_map.h>
#include <cage-core/pointerRangeHolder.h>

#include <algorithm>

namespace cage
{
	namespace
	{
		struct Item
		{
			const Collider *c = nullptr;
			const transform t;

			Item(const Collider *c, const transform &t) : c(c), t(t)
			{}
		};

		class CollisionDataImpl : public CollisionStructure
		{
		public:
			cage::unordered_map<uint32, Item> allItems;
			Holder<SpatialStructure> spatial;

			CollisionDataImpl(const CollisionStructureCreateConfig &config)
			{
				spatial = newSpatialStructure(config.spatialConfig ? *config.spatialConfig : SpatialStructureCreateConfig());
			}

			~CollisionDataImpl()
			{
				clear();
			}
		};

		class CollisionQueryImpl : public CollisionQuery
		{
		public:
			const CollisionDataImpl *const data;
			Holder<SpatialQuery> spatial;
			uint32 resultName = m;
			real resultFractionBefore = real::Nan();
			real resultFractionContact = real::Nan();
			Holder<PointerRange<CollisionPair>> resultPairs;

			CollisionQueryImpl(const CollisionDataImpl *data) : data(data)
			{
				spatial = newSpatialQuery(data->spatial.get());
			}

			void clear()
			{
				resultName = m;
				resultFractionBefore = resultFractionContact = real::Nan();
				resultPairs.clear();
			}

			bool query(const Collider *collider, const transform &t)
			{
				clear();
				spatial->intersection(collider->box() * t); // broad phase
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					Holder<PointerRange<CollisionPair>> tmpPairs;
					if (collisionDetection(collider, item.c, t, item.t, tmpPairs)) // exact phase
					{
						CAGE_ASSERT(!tmpPairs.empty());
						std::swap(tmpPairs, resultPairs);
						resultName = nameIt;
						break;
					}
				}
				CAGE_ASSERT(resultPairs.empty() != (resultName != m));
				return !resultPairs.empty();
			}

			bool query(const Collider *collider, const transform &t1, const transform &t2)
			{
				clear();
				real best = real::Infinity();
				spatial->intersection(collider->box() * t1 + collider->box() * t2); // broad phase
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					real fractBefore, fractContact;
					Holder<PointerRange<CollisionPair>> tmpPairs;
					if (!collisionDetection(collider, item.c, t1, item.t, t2, item.t, fractBefore, fractContact, tmpPairs)) // exact phase
						continue;
					if (fractContact < best)
					{
						CAGE_ASSERT(!tmpPairs.empty());
						std::swap(tmpPairs, resultPairs);
						resultFractionBefore = fractBefore;
						resultFractionContact = best = fractContact;
						resultName = nameIt;
					}
				}
				CAGE_ASSERT(resultPairs.empty() != resultFractionContact.valid());
				CAGE_ASSERT(resultPairs.empty() != (resultName != m));
				return !resultPairs.empty();
			}

			template<class T>
			void findPairs(const T &shape)
			{
				const Item &item = data->allItems.at(resultName);
				uint32 i = 0;
				PointerRangeHolder<CollisionPair> pairs;
				for (const triangle &t : item.c->triangles())
				{
					if (intersects(shape, t * item.t))
					{
						CollisionPair p;
						p.a = m;
						p.b = i;
						pairs.push_back(p);
					}
					i++;
				}
				CAGE_ASSERT(!pairs.empty());
				resultPairs = templates::move(pairs);
			}

			template<class T>
			bool query(const T &shape)
			{
				clear();
				spatial->intersection(aabb(shape)); // broad phase
				bool found = false;
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					if (intersects(shape, item.c, item.t)) // exact phase
					{
						resultName = nameIt;
						found = true;
						break;
					}
				}
				if (found)
				{
					findPairs(shape);
					return true;
				}
				return false;
			}
		};

		template<>
		bool CollisionQueryImpl::query(const line &shape)
		{
			CAGE_ASSERT(shape.normalized());
			clear();
			spatial->intersection(aabb(shape)); // broad phase
			real best = real::Infinity();
			for (uint32 nameIt : spatial->result())
			{
				const Item &item = data->allItems.at(nameIt);
				vec3 p = intersection(shape, item.c, item.t); // exact phase
				//CAGE_ASSERT(intersects(shape, item.c, item.t) == p.valid());
				if (!p.valid())
					continue;
				real d = dot(p - shape.origin, shape.direction);
				if (d < best)
				{
					best = d;
					resultName = nameIt;
				}
			}
			if (best.finite())
			{
				findPairs(shape);
				return true;
			}
			return false;
		}
	}

	uint32 CollisionQuery::name() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl*)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		return impl->resultName;
	}

	real CollisionQuery::fractionBefore() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl*)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		return impl->resultFractionBefore;
	}

	real CollisionQuery::fractionContact() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl*)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		return impl->resultFractionContact;
	}

	PointerRange<CollisionPair> CollisionQuery::collisionPairs() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl*)this;
		return impl->resultPairs;
	}

	void CollisionQuery::collider(const Collider *&c, transform &t) const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl*)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		auto r = impl->data->allItems.at(impl->resultName);
		c = r.c;
		t = r.t;
	}

	bool CollisionQuery::query(const Collider *collider, const transform &t)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(collider, t);
	}

	bool CollisionQuery::query(const Collider *collider, const transform &t1, const transform &t2)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(collider, t1, t2);
	}

	bool CollisionQuery::query(const line &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const triangle &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const plane &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const sphere &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const aabb &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->query(shape);
	}

	void CollisionStructure::update(uint32 name, const Collider *collider, const transform &t)
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		remove(name);
		impl->allItems.emplace(name, Item(collider, t));
		impl->spatial->update(name, collider->box() * t);
	}

	void CollisionStructure::remove(uint32 name)
	{
		CAGE_ASSERT(name != m);
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		impl->spatial->remove(name);
		impl->allItems.erase(name);
	}

	void CollisionStructure::clear()
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		impl->allItems.clear();
		impl->spatial->clear();
	}

	void CollisionStructure::rebuild()
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		impl->spatial->rebuild();
	}

	Holder<CollisionStructure> newCollisionStructure(const CollisionStructureCreateConfig &config)
	{
		return detail::systemArena().createImpl<CollisionStructure, CollisionDataImpl>(config);
	}

	Holder<CollisionQuery> newCollisionQuery(const CollisionStructure *data)
	{
		return detail::systemArena().createImpl<CollisionQuery, CollisionQueryImpl>((CollisionDataImpl*)data);
	}
}
