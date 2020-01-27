#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/collision.h>
#include <cage-core/collisionMesh.h>
#include <cage-core/spatial.h>
#include <cage-core/unordered_map.h>

#include <vector>
#include <algorithm>

namespace cage
{
	namespace
	{
		struct Item : public transform
		{
			const CollisionMesh *collider;

			Item(const CollisionMesh *collider, const transform &t) : transform(t), collider(collider)
			{}
		};

		class CollisionDataImpl : public CollisionData
		{
		public:
			cage::unordered_map<uint32, Item> allItems;
			Holder<SpatialData> spatial;
			const uint32 maxCollisionPairs;

			static SpatialDataCreateConfig spatialConfig(const CollisionDataCreateConfig &config)
			{
				if (config.spatialConfig)
					return *config.spatialConfig;
				return SpatialDataCreateConfig();
			}

			CollisionDataImpl(const CollisionDataCreateConfig &config) : maxCollisionPairs(config.maxCollisionPairs)
			{
				spatial = newSpatialData(spatialConfig(config));
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
			uint32 resultName;
			real resultFractionBefore;
			real resultFractionContact;
			std::vector<CollisionPair> resultPairs;
			std::vector<CollisionPair> tmpPairs;

			CollisionQueryImpl(const CollisionDataImpl *data) : data(data), resultName(0)
			{
				spatial = newSpatialQuery(data->spatial.get());
			}

			void query(const CollisionMesh *collider, const transform &t1, const transform &t2)
			{
				resultName = 0;
				resultFractionBefore = resultFractionContact = real::Nan();
				resultPairs.clear();
				real best = real::Infinity();
				spatial->intersection(collider->box() * t1 + collider->box() * t2);
				for (uint32 nameIt : spatial->result())
				{
					tmpPairs.resize(data->maxCollisionPairs);
					const Item &item = data->allItems.at(nameIt);
					real fractBefore, fractContact;
					uint32 res = collisionDetection(collider, item.collider, t1, item, t2, item, fractBefore, fractContact, tmpPairs.data(), numeric_cast<uint32>(tmpPairs.size()));
					if (res > 0 && fractContact < best)
					{
						tmpPairs.resize(res);
						tmpPairs.swap(resultPairs);
						resultFractionBefore = fractBefore;
						resultFractionContact = best = fractContact;
						resultName = nameIt;
					}
				}
			}

			template<class T>
			void spatialIntersection(const T &shape)
			{
				spatial->intersection(aabb(shape));
			}

			template<class T>
			void query(const T &shape)
			{
				resultName = 0;
				resultFractionBefore = resultFractionContact = real::Nan();
				resultPairs.clear();
				real best = real::Infinity();
				spatialIntersection(shape);
				for (uint32 nameIt : spatial->result())
				{
					const Item &item = data->allItems.at(nameIt);
					if (intersects(shape, item.collider, item))
					{
						real d = distance(shape, item.collider, item);
						if (d < best)
						{
							best = d;
							resultName = nameIt;
						}
					}
				}
				if (resultName)
				{
					const Item &item = data->allItems.at(resultName);
					uint32 i = 0;
					for (const triangle &t : item.collider->triangles())
					{
						if (intersects(shape, t * item))
							resultPairs.emplace_back(m, i);
						i++;
					}
					CAGE_ASSERT(!resultPairs.empty());
				}
			}
		};

		template<>
		void CollisionQueryImpl::spatialIntersection(const plane &shape)
		{
			spatial->intersection(aabb::Universe());
		}
	}

	uint32 CollisionQuery::name() const
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->resultName;
	}

	real CollisionQuery::fractionBefore() const
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->resultFractionBefore;
	}

	real CollisionQuery::fractionContact() const
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->resultFractionContact;
	}

	PointerRange<CollisionPair> CollisionQuery::collisionPairs() const
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		return impl->resultPairs;
	}

	void CollisionQuery::collider(const CollisionMesh *&c, transform &t) const
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		auto r = impl->data->allItems.at(impl->resultName);
		c = r.collider;
		t = r;
	}

	void CollisionQuery::query(const CollisionMesh *collider, const transform &t)
	{
		query(collider, t, t);
	}

	void CollisionQuery::query(const CollisionMesh *collider, const transform &t1, const transform &t2)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		impl->query(collider, t1, t2);
	}

	void CollisionQuery::query(const line &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		impl->query(shape);
	}

	void CollisionQuery::query(const triangle &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		impl->query(shape);
	}

	void CollisionQuery::query(const plane &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		impl->query(shape);
	}

	void CollisionQuery::query(const sphere &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		impl->query(shape);
	}

	void CollisionQuery::query(const aabb &shape)
	{
		CollisionQueryImpl *impl = (CollisionQueryImpl*)this;
		impl->query(shape);
	}

	void CollisionData::update(uint32 name, const CollisionMesh *collider, const transform &t)
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		remove(name);
		impl->allItems.emplace(name, Item(collider, t));
		impl->spatial->update(name, collider->box() * mat4(t));
	}

	void CollisionData::remove(uint32 name)
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		impl->spatial->remove(name);
		impl->allItems.erase(name);
	}

	void CollisionData::clear()
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		impl->allItems.clear();
		impl->spatial->clear();
	}

	void CollisionData::rebuild()
	{
		CollisionDataImpl *impl = (CollisionDataImpl*)this;
		impl->spatial->rebuild();
	}

	Holder<CollisionData> newCollisionData(const CollisionDataCreateConfig &config)
	{
		return detail::systemArena().createImpl<CollisionData, CollisionDataImpl>(config);
	}

	Holder<CollisionQuery> newCollisionQuery(const CollisionData *data)
	{
		return detail::systemArena().createImpl<CollisionQuery, CollisionQueryImpl>((CollisionDataImpl*)data);
	}
}
