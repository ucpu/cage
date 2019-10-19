#include <vector>
#include <algorithm>
#include <robin_hood.h>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/collision.h>
#include <cage-core/collisionMesh.h>
#include <cage-core/spatial.h>

namespace cage
{
	namespace
	{
		struct itemStruct : public transform
		{
			const collisionMesh *collider;

			itemStruct(const collisionMesh *collider, const transform &t) : transform(t), collider(collider)
			{}
		};

		class collisionDataImpl : public collisionData
		{
		public:
			robin_hood::unordered_map<uint32, itemStruct> allItems;
			holder<spatialData> spatial;
			const uint32 maxCollisionPairs;

			static spatialDataCreateConfig spatialConfig(const collisionDataCreateConfig &config)
			{
				if (config.spatialConfig)
					return *config.spatialConfig;
				return spatialDataCreateConfig();
			}

			collisionDataImpl(const collisionDataCreateConfig &config) : maxCollisionPairs(config.maxCollisionPairs)
			{
				spatial = newSpatialData(spatialConfig(config));
			}

			~collisionDataImpl()
			{
				clear();
			}
		};

		class collisionQueryImpl : public collisionQuery
		{
		public:
			const collisionDataImpl *const data;
			holder<spatialQuery> spatial;
			uint32 resultName;
			real resultFractionBefore;
			real resultFractionContact;
			std::vector<collisionPair> resultPairs;
			std::vector<collisionPair> tmpPairs;

			collisionQueryImpl(const collisionDataImpl *data) : data(data), resultName(0)
			{
				spatial = newSpatialQuery(data->spatial.get());
			}

			void query(const collisionMesh *collider, const transform &t1, const transform &t2)
			{
				resultName = 0;
				resultFractionBefore = resultFractionContact = real::Nan();
				resultPairs.clear();
				real best = real::Infinity();
				spatial->intersection(collider->box() * t1 + collider->box() * t2);
				for (uint32 nameIt : spatial->result())
				{
					tmpPairs.resize(data->maxCollisionPairs);
					const itemStruct &item = data->allItems.at(nameIt);
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
					const itemStruct &item = data->allItems.at(nameIt);
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
					const itemStruct &item = data->allItems.at(resultName);
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
		void collisionQueryImpl::spatialIntersection(const plane &shape)
		{
			spatial->intersection(aabb::Universe());
		}
	}

	uint32 collisionQuery::name() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultName;
	}

	real collisionQuery::fractionBefore() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultFractionBefore;
	}

	real collisionQuery::fractionContact() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultFractionContact;
	}

	uint32 collisionQuery::collisionPairsCount() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return numeric_cast<uint32>(impl->resultPairs.size());
	}

	const collisionPair *collisionQuery::collisionPairsData() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultPairs.data();
	}

	pointerRange<const collisionPair> collisionQuery::collisionPairs() const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		return impl->resultPairs;
	}

	void collisionQuery::collider(const collisionMesh *&c, transform &t) const
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		auto r = impl->data->allItems.at(impl->resultName);
		c = r.collider;
		t = r;
	}

	void collisionQuery::query(const collisionMesh *collider, const transform &t)
	{
		query(collider, t, t);
	}

	void collisionQuery::query(const collisionMesh *collider, const transform &t1, const transform &t2)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(collider, t1, t2);
	}

	void collisionQuery::query(const line &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQuery::query(const triangle &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQuery::query(const plane &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQuery::query(const sphere &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionQuery::query(const aabb &shape)
	{
		collisionQueryImpl *impl = (collisionQueryImpl*)this;
		impl->query(shape);
	}

	void collisionData::update(uint32 name, const collisionMesh *collider, const transform &t)
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		remove(name);
		impl->allItems.emplace(name, itemStruct(collider, t));
		impl->spatial->update(name, collider->box() * mat4(t));
	}

	void collisionData::remove(uint32 name)
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		impl->spatial->remove(name);
		impl->allItems.erase(name);
	}

	void collisionData::clear()
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		impl->allItems.clear();
		impl->spatial->clear();
	}

	void collisionData::rebuild()
	{
		collisionDataImpl *impl = (collisionDataImpl*)this;
		impl->spatial->rebuild();
	}

	collisionDataCreateConfig::collisionDataCreateConfig() : spatialConfig(nullptr), maxCollisionPairs(100)
	{}

	holder<collisionData> newCollisionData(const collisionDataCreateConfig &config)
	{
		return detail::systemArena().createImpl<collisionData, collisionDataImpl>(config);
	}

	holder<collisionQuery> newCollisionQuery(const collisionData *data)
	{
		return detail::systemArena().createImpl<collisionQuery, collisionQueryImpl>((collisionDataImpl*)data);
	}
}
