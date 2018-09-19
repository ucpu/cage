#include <vector>
#include <algorithm>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/hashTable.h>
#include <cage-core/collision.h>
#include <cage-core/collider.h>
#include <cage-core/spatial.h>

namespace cage
{
	namespace
	{
		struct itemStruct : public transform
		{
			const colliderClass *collider;

			itemStruct(const colliderClass *collider, const transform &t) : transform(t), collider(collider) {}
		};

		class collisionDataImpl : public collisionDataClass
		{
		public:
			memoryArenaGrowing<memoryAllocatorPolicyPool<sizeof(itemStruct)>, memoryConcurrentPolicyNone> pool;
			memoryArena arena;
			typedef holder<cage::hashTableClass<itemStruct>> itemsMapType;
			itemsMapType allItems;
			holder<spatialDataClass> spatial;
			const uint32 maxCollisionPairs;

			static spatialDataCreateConfig spatialConfig(const collisionDataCreateConfig &config)
			{
				if (config.spatialConfig)
					return *config.spatialConfig;
				return spatialDataCreateConfig();
			}

			collisionDataImpl(const collisionDataCreateConfig &config) :
				pool(spatialConfig(config).memory), arena(&pool), maxCollisionPairs(config.maxCollisionPairs)
			{
				uint32 maxItems = numeric_cast<uint32>(spatialConfig(config).memory / sizeof(itemStruct));
				allItems = newHashTable<itemStruct>(min(maxItems, 100u), maxItems);
				spatial = newSpatialData(spatialConfig(config));
			}

			~collisionDataImpl()
			{
				clear();
			}
		};

		class collisionQueryImpl : public collisionQueryClass
		{
		public:
			const collisionDataImpl *const data;
			holder<spatialQueryClass> spatial;
			uint32 resultName;
			real resultFractionBefore;
			real resultFractionContact;
			std::vector<collisionPairStruct> resultPairs;
			std::vector<collisionPairStruct> tmpPairs;

			collisionQueryImpl(const collisionDataImpl *data) : data(data), resultName(0)
			{
				spatial = newSpatialQuery(data->spatial.get());
			}

			void query(const colliderClass *collider, const transform &t1, const transform &t2)
			{
				resultName = 0;
				resultFractionBefore = resultFractionContact = real::Nan;
				resultPairs.clear();
				real best = real::PositiveInfinity;
				spatial->intersection(collider->box() * t1 + collider->box() * t2);
				for (uint32 nameIt : spatial->result())
				{
					tmpPairs.resize(data->maxCollisionPairs);
					itemStruct *item = data->allItems->get(nameIt, false);
					real fractBefore, fractContact;
					uint32 res = collisionDetection(collider, item->collider, t1, *item, t2, *item, fractBefore, fractContact, tmpPairs.data(), numeric_cast<uint32>(tmpPairs.size()));
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
				resultFractionBefore = resultFractionContact = real::Nan;
				resultPairs.clear();
				real best = real::PositiveInfinity;
				spatialIntersection(shape);
				for (uint32 nameIt : spatial->result())
				{
					itemStruct *item = data->allItems->get(nameIt, false);
					if (intersects(shape, item->collider, *item))
					{
						real d = distance(shape, item->collider, *item);
						if (d < best)
						{
							best = d;
							resultName = nameIt;
						}
					}
				}
				if (resultName)
				{
					itemStruct *item = data->allItems->get(resultName, false);
					uint32 i = 0;
					for (const triangle &t : item->collider->triangles())
					{
						if (intersects(shape, t * *item))
							resultPairs.emplace_back(-1, i);
						i++;
					}
					CAGE_ASSERT_RUNTIME(!resultPairs.empty());
				}
			}
		};

		template<>
		void collisionQueryImpl::spatialIntersection(const plane &shape)
		{
			spatial->intersection(aabb::Universe);
		}
	}

	uint32 collisionQueryClass::name() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultName;
	}

	real collisionQueryClass::fractionBefore() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultFractionBefore;
	}

	real collisionQueryClass::fractionContact() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultFractionContact;
	}

	uint32 collisionQueryClass::collisionPairsCount() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return numeric_cast<uint32>(impl->resultPairs.size());
	}

	const collisionPairStruct *collisionQueryClass::collisionPairsData() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultPairs.data();
	}

	pointerRange<const collisionPairStruct> collisionQueryClass::collisionPairs() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultPairs;
	}

	void collisionQueryClass::collider(const colliderClass *&c, transform &t) const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->resultName);
		auto r = impl->data->allItems->get(impl->resultName, false);
		c = r->collider;
		t = *r;
	}

	void collisionQueryClass::query(const colliderClass *collider, const transform &t)
	{
		query(collider, t, t);
	}

	void collisionQueryClass::query(const colliderClass *collider, const transform &t1, const transform &t2)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(collider, t1, t2);
	}

	void collisionQueryClass::query(const line &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQueryClass::query(const triangle &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQueryClass::query(const plane &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQueryClass::query(const sphere &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQueryClass::query(const aabb &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionDataClass::update(uint32 name, const colliderClass *collider, const transform &t)
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		remove(name);
		itemStruct *it = impl->arena.createObject<itemStruct>(collider, t);
		impl->allItems->add(name, it);
		impl->spatial->update(name, collider->box() * mat4(t));
	}

	void collisionDataClass::remove(uint32 name)
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		impl->spatial->remove(name);
		auto item = impl->allItems->get(name, true);
		if (!item)
			return;
		impl->allItems->remove(name);
		impl->arena.deallocate(item);
	}

	void collisionDataClass::clear()
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		impl->arena.flush();
		impl->allItems->clear();
		impl->spatial->clear();
	}

	void collisionDataClass::rebuild()
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		impl->spatial->rebuild();
	}

	collisionDataCreateConfig::collisionDataCreateConfig() : spatialConfig(nullptr), maxCollisionPairs(100)
	{}

	holder<collisionDataClass> newCollisionData(const collisionDataCreateConfig &config)
	{
		return detail::systemArena().createImpl<collisionDataClass, collisionDataImpl>(config);
	}

	holder<collisionQueryClass> newCollisionQuery(const collisionDataClass *data)
	{
		return detail::systemArena().createImpl<collisionQueryClass, collisionQueryImpl>((collisionDataImpl*)data);
	}
}
