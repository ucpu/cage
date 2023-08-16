#include <algorithm>

#include <unordered_dense.h>

#include <cage-core/collider.h>
#include <cage-core/collisionStructure.h>
#include <cage-core/geometry.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/spatialStructure.h>

namespace cage
{
	namespace
	{
		struct Item
		{
			Holder<const Collider> c;
			Transform t;
		};

		class CollisionDataImpl : public CollisionStructure
		{
		public:
			ankerl::unordered_dense::map<uint32, Item> allItems;
			Holder<SpatialStructure> spatial;

			CollisionDataImpl(const CollisionStructureCreateConfig &config) { spatial = newSpatialStructure(config.spatialConfig ? *config.spatialConfig : SpatialStructureCreateConfig()); }

			~CollisionDataImpl() { clear(); }
		};

		class CollisionQueryImpl : public CollisionQuery
		{
		public:
			const Holder<const CollisionDataImpl> data;
			Holder<SpatialQuery> spatial;
			uint32 resultName = m;
			Real resultFractionBefore = Real::Nan();
			Real resultFractionContact = Real::Nan();
			Holder<PointerRange<CollisionPair>> resultPairs;

			CollisionQueryImpl(Holder<const CollisionDataImpl> data) : data(std::move(data)) { spatial = newSpatialQuery(this->data->spatial.share()); }

			void clear()
			{
				resultName = m;
				resultFractionBefore = resultFractionContact = Real::Nan();
				resultPairs.clear();
			}

			bool query(const Collider *collider, const Transform &t)
			{
				clear();
				spatial->intersection(collider->box() * t); // broad phase
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					CollisionDetectionConfig p(collider, +item.c, t, item.t);
					if (collisionDetection(p)) // exact phase
					{
						CAGE_ASSERT(!p.collisionPairs.empty());
						std::swap(p.collisionPairs, resultPairs);
						resultName = nameIt;
						break;
					}
				}
				CAGE_ASSERT(resultPairs.empty() != (resultName != m));
				return !resultPairs.empty();
			}

			bool query(const Collider *collider, const Transform &t1, const Transform &t2)
			{
				clear();
				Real best = Real::Infinity();
				spatial->intersection(collider->box() * t1 + collider->box() * t2); // broad phase
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					CollisionDetectionConfig p(collider, +item.c);
					p.at1 = t1;
					p.at2 = t2;
					p.bt1 = item.t;
					p.bt2 = item.t;
					if (!collisionDetection(p)) // exact phase
						continue;
					if (p.fractionContact < best)
					{
						CAGE_ASSERT(!p.collisionPairs.empty());
						std::swap(p.collisionPairs, resultPairs);
						resultFractionBefore = p.fractionBefore;
						resultFractionContact = best = p.fractionContact;
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
				for (const Triangle &t : item.c->triangles())
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
				resultPairs = std::move(pairs);
			}

			template<class T>
			bool query(const T &shape)
			{
				clear();
				spatial->intersection(Aabb(shape)); // broad phase
				bool found = false;
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					if (intersects(shape, +item.c, item.t)) // exact phase
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
		bool CollisionQueryImpl::query(const Line &shape)
		{
			CAGE_ASSERT(shape.normalized());
			clear();
			spatial->intersection(Aabb(shape)); // broad phase
			Real best = Real::Infinity();
			for (uint32 nameIt : spatial->result())
			{
				const Item &item = data->allItems.at(nameIt);
				Vec3 p = intersection(shape, +item.c, item.t); // exact phase
				if (!p.valid())
					continue;
				Real d = dot(p - shape.origin, shape.direction);
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
		const CollisionQueryImpl *impl = (const CollisionQueryImpl *)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		return impl->resultName;
	}

	Real CollisionQuery::fractionBefore() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl *)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		return impl->resultFractionBefore;
	}

	Real CollisionQuery::fractionContact() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl *)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		return impl->resultFractionContact;
	}

	PointerRange<CollisionPair> CollisionQuery::collisionPairs() const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl *)this;
		return impl->resultPairs;
	}

	void CollisionQuery::collider(Holder<const Collider> &c, Transform &t) const
	{
		const CollisionQueryImpl *impl = (const CollisionQueryImpl *)this;
		CAGE_ASSERT(!impl->resultPairs.empty());
		const auto &r = impl->data->allItems.at(impl->resultName);
		c = r.c.share();
		t = r.t;
	}

	bool CollisionQuery::query(const Collider *collider, const Transform &t)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(collider, t);
	}

	bool CollisionQuery::query(const Collider *collider, const Transform &t1, const Transform &t2)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(collider, t1, t2);
	}

	bool CollisionQuery::query(const Line &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const Triangle &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const Plane &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const Sphere &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const Aabb &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const Cone &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	bool CollisionQuery::query(const Frustum &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl *)this;
		return impl->query(shape);
	}

	void CollisionStructure::update(uint32 name, Holder<const Collider> collider, const Transform &t)
	{
		CollisionDataImpl *impl = (CollisionDataImpl *)this;
		remove(name);
		impl->spatial->update(name, collider->box() * t);
		impl->allItems.emplace(name, Item{ std::move(collider), t });
	}

	void CollisionStructure::remove(uint32 name)
	{
		CAGE_ASSERT(name != m);
		CollisionDataImpl *impl = (CollisionDataImpl *)this;
		impl->spatial->remove(name);
		impl->allItems.erase(name);
	}

	void CollisionStructure::clear()
	{
		CollisionDataImpl *impl = (CollisionDataImpl *)this;
		impl->allItems.clear();
		impl->spatial->clear();
	}

	void CollisionStructure::rebuild()
	{
		CollisionDataImpl *impl = (CollisionDataImpl *)this;
		impl->spatial->rebuild();
	}

	Holder<CollisionStructure> newCollisionStructure(const CollisionStructureCreateConfig &config)
	{
		return systemMemory().createImpl<CollisionStructure, CollisionDataImpl>(config);
	}

	Holder<CollisionQuery> newCollisionQuery(Holder<const CollisionStructure> structure)
	{
		return systemMemory().createImpl<CollisionQuery, CollisionQueryImpl>(std::move(structure).cast<const CollisionDataImpl>());
	}
}
