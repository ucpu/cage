#include <vector>
#include <algorithm>
#include <atomic>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/utility/hashTable.h>
#include <cage-core/utility/spatial.h>

namespace cage
{
	namespace
	{
		struct itemBase
		{
			virtual operator aabb() const = 0;
			virtual bool intersects(const line &other) = 0;
			virtual bool intersects(const triangle &other) = 0;
			virtual bool intersects(const sphere &other) = 0;
			virtual bool intersects(const aabb &other) = 0;
		};

		template<class T> struct itemShape : public itemBase, public T
		{
			itemShape(const T &other) : T(other) {}
			virtual operator aabb() const { return aabb(*this); }
			virtual bool intersects(const line &other) { return cage::intersects(*this, other); };
			virtual bool intersects(const triangle &other) { return cage::intersects(*this, other); };
			virtual bool intersects(const sphere &other) { return cage::intersects(*this, other); };
			virtual bool intersects(const aabb &other) { return cage::intersects(*this, other); };
		};

		union itemUnion
		{
			itemShape<line> a;
			itemShape<triangle> b;
			itemShape<plane> c;
			itemShape<sphere> d;
			itemShape<aabb> e;
		};

		static const uintPtr maxSize = sizeof(itemUnion);

		class spatialDataImpl : public spatialDataClass
		{
		public:
			memoryArenaGrowing<memoryAllocatorPolicyPool<maxSize>, memoryConcurrentPolicyNone> pool;
			memoryArena arena;
			typedef holder<cage::hashTableClass<itemBase>> itemsMapType;
			itemsMapType allItems;
			std::vector<itemsMapType> itemsGrid;
			aabb gridUniverse;
			uint32 rx, ry, rz;
			real gridResolutionCoefficient;
			std::atomic<bool> dirty;

			spatialDataImpl(const spatialDataCreateConfig &config) :
				pool(config.memory), arena(&pool),
				rx(0), ry(0), rz(0), gridResolutionCoefficient(config.gridResolutionCoefficient), dirty(false)
			{
				uint32 maxItems = numeric_cast<uint32>(config.memory / maxSize);
				allItems = newHashTable<itemBase>(min(maxItems, 100u), maxItems);
			}

			~spatialDataImpl()
			{
				clear();
			}

			static uint32 gridRange(real search, real left, real right, uint32 res)
			{
				if (search < left)
					return 0;
				if (search >= right)
					return res - 1;
				uint32 result = numeric_cast<uint32>((search - left) / (right - left) * (res - 1));
				CAGE_ASSERT_RUNTIME(result >= 0 && result < res, result, res);
				return result;
			}

			void gridRange(const aabb &box, uint32 &xMin, uint32 &xMax, uint32 &yMin, uint32 &yMax, uint32 &zMin, uint32 &zMax) const
			{
				xMin = gridRange(box.a[0], gridUniverse.a[0], gridUniverse.b[0], rx);
				yMin = gridRange(box.a[1], gridUniverse.a[1], gridUniverse.b[1], ry);
				zMin = gridRange(box.a[2], gridUniverse.a[2], gridUniverse.b[2], rz);
				xMax = gridRange(box.b[0], gridUniverse.a[0], gridUniverse.b[0], rx) + 1;
				yMax = gridRange(box.b[1], gridUniverse.a[1], gridUniverse.b[1], ry) + 1;
				zMax = gridRange(box.b[2], gridUniverse.a[2], gridUniverse.b[2], rz) + 1;
			}

			void rebuild()
			{
				rx = ry = rz = 0;
				gridUniverse = aabb();
				if (allItems->count() == 0)
				{
					dirty = false;
					return;
				}

				for (auto it = allItems->begin(), et = allItems->end(); it != et; it++)
					gridUniverse += aabb(*it->second);
				decideResolution();

				itemsGrid.resize(rx * ry * rz);
				uint32 itemsPerCell = numeric_cast<uint32>(allItems->count() / itemsGrid.size() + 1) * 2;
				itemsPerCell = min(itemsPerCell, allItems->count());
				for (auto &&it : itemsGrid)
					it = newHashTable<itemBase>(itemsPerCell, allItems->count());
				for (auto it = allItems->begin(), et = allItems->end(); it != et; it++)
				{
					uint32 xMin, xMax, yMin, yMax, zMin, zMax;
					gridRange(aabb(*it->second), xMin, xMax, yMin, yMax, zMin, zMax);
					for (uint32 z = zMin; z < zMax; z++)
						for (uint32 y = yMin; y < yMax; y++)
							for (uint32 x = xMin; x < xMax; x++)
								itemsGrid[((z * ry) + y) * rx + x]->add(it->first, it->second);
				}

				dirty = false;
			}

			void decideResolution()
			{
				vec3 d = gridUniverse.size();
				real scale = gridResolutionCoefficient * allItems->count() / (
					(d[0] > 0 ? d[0] : 1) *
					(d[1] > 0 ? d[1] : 1) *
					(d[2] > 0 ? d[2] : 1)
					);
				scale = pow(scale, 0.3333333);
				rx = d[0] > 0 ? max(numeric_cast<uint32>(d[0] * scale), 1u) : 1;
				ry = d[1] > 0 ? max(numeric_cast<uint32>(d[1] * scale), 1u) : 1;
				rz = d[2] > 0 ? max(numeric_cast<uint32>(d[2] * scale), 1u) : 1;
			}
		};

		class spatialQueryImpl : public spatialQueryClass
		{
		public:
			const spatialDataImpl *const data;
			std::vector<uint32> resultNames;

			spatialQueryImpl(const spatialDataImpl *data) : data(data)
			{
				resultNames.reserve(100);
			}

			void clear()
			{
				CAGE_ASSERT_RUNTIME(!data->dirty);
				resultNames.clear();
			}

			template<class T> void intersection(const T &other)
			{
				CAGE_ASSERT_RUNTIME(!data->dirty);
				clear();
				if (data->rx == 0)
					return;
				uint32 xMin, xMax, yMin, yMax, zMin, zMax;
				data->gridRange(aabb(other), xMin, xMax, yMin, yMax, zMin, zMax);
				for (uint32 z = zMin; z < zMax; z++)
				{
					for (uint32 y = yMin; y < yMax; y++)
					{
						for (uint32 x = xMin; x < xMax; x++)
						{
							const auto &mp = data->itemsGrid[((z * data->ry) + y) * data->rx + x];
							for (auto it = mp->begin(), et = mp->end(); it != et; it++)
								if (it->second->intersects(other))
									resultNames.push_back(it->first);
						}
					}
				}
				std::sort(resultNames.begin(), resultNames.end());
				auto et = std::unique(resultNames.begin(), resultNames.end());
				resultNames.resize(et - resultNames.begin());
			}
		};
	}

	uint32 spatialQueryClass::resultCount() const
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		return numeric_cast<uint32>(impl->resultNames.size());
	}

	const uint32 *spatialQueryClass::resultArray() const
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		return impl->resultNames.data();
	}

	pointerRange<const uint32> spatialQueryClass::result() const
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		return impl->resultNames;
	}

	void spatialQueryClass::intersection(const line &shape)
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		impl->intersection(shape);
	}

	void spatialQueryClass::intersection(const triangle &shape)
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		impl->intersection(shape);
	}

	void spatialQueryClass::intersection(const sphere &shape)
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		impl->intersection(shape);
	}

	void spatialQueryClass::intersection(const aabb &shape)
	{
		spatialQueryImpl *impl = (spatialQueryImpl*)this;
		impl->intersection(shape);
	}

	void spatialDataClass::update(uint32 name, const line &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.isPoint() || other.isSegment());
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->allItems->add(name, impl->arena.createObject<itemShape<line>>(other));
	}

	void spatialDataClass::update(uint32 name, const triangle &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.area() < real::PositiveInfinity);
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->allItems->add(name, impl->arena.createObject<itemShape<triangle>>(other));
	}

	void spatialDataClass::update(uint32 name, const sphere &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.volume() < real::PositiveInfinity);
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->allItems->add(name, impl->arena.createObject<itemShape<sphere>>(other));
	}

	void spatialDataClass::update(uint32 name, const aabb &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.volume() < real::PositiveInfinity);
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->allItems->add(name, impl->arena.createObject<itemShape<aabb>>(other));
	}

	void spatialDataClass::remove(uint32 name)
	{
		spatialDataImpl *impl = (spatialDataImpl*)this;
		impl->dirty = true;
		auto item = impl->allItems->get(name, true);
		if (!item)
			return;
		impl->allItems->remove(name);
		impl->arena.deallocate(item);
	}

	void spatialDataClass::clear()
	{
		spatialDataImpl *impl = (spatialDataImpl*)this;
		impl->dirty = true;
		impl->arena.flush();
		impl->allItems->clear();
	}

	void spatialDataClass::rebuild()
	{
		spatialDataImpl *impl = (spatialDataImpl*)this;
		impl->rebuild();
	}

	spatialDataCreateConfig::spatialDataCreateConfig() : memory(1024 * 1024 * 64), gridResolutionCoefficient(0.2)
	{}

	holder<spatialDataClass> newSpatialData(const spatialDataCreateConfig &config)
	{
		return detail::systemArena().createImpl<spatialDataClass, spatialDataImpl>(config);
	}

	holder<spatialQueryClass> newSpatialQuery(const spatialDataClass *data)
	{
		return detail::systemArena().createImpl<spatialQueryClass, spatialQueryImpl>((spatialDataImpl*)data);
	}
}
