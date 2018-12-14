#include <vector>
#include <algorithm>
#include <atomic>
#include <array>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/hashTable.h>
#include <cage-core/spatial.h>

namespace cage
{
	namespace
	{
		struct itemBase
		{
			aabb box;
			vec3 center;
			const uint32 name;
			
			virtual aabb getBox() const = 0;
			virtual bool intersects(const line &other) = 0;
			virtual bool intersects(const triangle &other) = 0;
			virtual bool intersects(const plane &other) = 0;
			virtual bool intersects(const sphere &other) = 0;
			virtual bool intersects(const aabb &other) = 0;

			itemBase(uint32 name) : name(name)
			{}

			void update()
			{
				box = getBox();
				center = box.center();
			}
		};

		template<class T>
		struct itemShape : public itemBase, public T
		{
			itemShape(uint32 name, const T &other) : itemBase(name), T(other)
			{
				update();
			}

			virtual aabb getBox() const { return aabb(*(T*)(this)); }
			virtual bool intersects(const line &other) { return cage::intersects(*this, other); };
			virtual bool intersects(const triangle &other) { return cage::intersects(*this, other); };
			virtual bool intersects(const plane &other) { return cage::intersects(*this, other); };
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

		struct nodeStruct
		{
			aabb box;
			sint32 a; // negative -> inner node, -a = index of left child; non-negative -> leaf node, a = offset into itemNames array
			sint32 b; // negative -> inner node, -b = index of right child; non-negative -> leaf node, b = items count
			
			nodeStruct(sint32 a, sint32 b) : a(a), b(b)
			{}
		};

		class spatialDataImpl : public spatialDataClass
		{
		public:
			static const uint32 binsCount = 10;
			memoryArenaIndexed<memoryArenaGrowing<memoryAllocatorPolicyPool<sizeof(itemUnion)>, memoryConcurrentPolicyNone>, sizeof(itemUnion)> itemsPool;
			memoryArena itemsArena;
			holder<cage::hashTableClass<itemBase>> itemsTable;
			std::atomic<bool> dirty;
			std::vector<nodeStruct> nodes;
			std::vector<itemBase*> indices;
			typedef std::vector<itemBase*>::iterator indicesIterator;
			std::array<aabb, binsCount> leftBinBoxes;
			std::array<aabb, binsCount> rightBinBoxes;

			spatialDataImpl(const spatialDataCreateConfig &config) : itemsPool(config.maxItems * sizeof(itemUnion)), itemsArena(&itemsPool), dirty(false)
			{
				itemsTable = newHashTable<itemBase>(min(config.maxItems, 100u), config.maxItems * 2); // times 2 to compensate for fill ratio
			}

			~spatialDataImpl()
			{
				clear();
			}

			static void sortHelpersByAxis(indicesIterator begin, indicesIterator end, uint32 axis)
			{
				std::sort(begin, end, [&](const itemBase *a, const itemBase *b) {
					return a->center[axis] < b->center[axis];
				});
			}

			static uint32 splitItems(uint32 splitIndex, uint32 itemsCount)
			{
				// for convenience, split index that corresponds to all items on left and no items on right is allowed here
				CAGE_ASSERT_RUNTIME(splitIndex < binsCount);
				return itemsCount * (splitIndex + 1) / binsCount;
			}

			aabb makeBox(indicesIterator begin, indicesIterator end)
			{
				aabb res;
				for (auto it = begin; it != end; it++)
					res += (*it)->box;
				return res;
			}

			void evaluateAxis(indicesIterator begin, indicesIterator end, uint32 axis, uint32 &bestAxis, uint32 &bestSplit, real &bestSah)
			{
				uint32 itemsCount = numeric_cast<uint32>(end - begin);
				// prepare bin boxes
				for (uint32 i = 0; i < binsCount - 1; i++)
					leftBinBoxes[i] = makeBox(begin + splitItems(i, itemsCount), begin + splitItems(i + 1, itemsCount));
				leftBinBoxes[binsCount - 1] = makeBox(begin + splitItems(binsCount - 2, itemsCount), end);
				// right to left sweep
				rightBinBoxes[binsCount - 1] = leftBinBoxes[binsCount - 1];
				for (uint32 i = binsCount - 1; i > 0; i--)
					rightBinBoxes[i - 1] = rightBinBoxes[i] + leftBinBoxes[i - 1];
				// left to right sweep
				for (uint32 i = 1; i < binsCount; i++)
					leftBinBoxes[i] += leftBinBoxes[i - 1];
				// compute sah
				for (uint32 i = 0; i < binsCount - 1; i++)
				{
					uint32 split = splitItems(i, itemsCount);
					real sahL = leftBinBoxes[i].surface() * split;
					real sahR = rightBinBoxes[i + 1].surface() * (itemsCount - split - 1);
					real sah = sahL + sahR;
					if (sah < bestSah)
					{
						bestAxis = axis;
						bestSplit = i;
						bestSah = sah;
					}
				}
			}

			void makeNodeBox(uint32 nodeIndex)
			{
				nodeStruct &node = nodes[nodeIndex];
				CAGE_ASSERT_RUNTIME(node.a >= 0 && node.b >= 0);
				node.box = makeBox(indices.begin() + node.a, indices.begin() + (node.a + node.b));
			}

			void rebuild(uint32 nodeIndex, uint32 nodeDepth, real parentSah)
			{
				nodeStruct &node = nodes[nodeIndex];
				CAGE_ASSERT_RUNTIME(node.a >= 0 && node.b >= 0);
				if (node.b < 10)
				{
					makeNodeBox(nodeIndex);
					return; // leaf node: too few primitives
				}
				uint32 bestAxis = -1;
				uint32 bestSplit = -1;
				real bestSah = real::PositiveInfinity;
				for (uint32 axis = 0; axis < 3; axis++)
				{
					sortHelpersByAxis(indices.begin() + node.a, indices.begin() + (node.a + node.b), axis);
					evaluateAxis(indices.begin() + node.a, indices.begin() + (node.a + node.b), axis, bestAxis, bestSplit, bestSah);
				}
				CAGE_ASSERT_RUNTIME(bestSah.valid());
				if (bestSah >= parentSah)
				{
					makeNodeBox(nodeIndex);
					return; // leaf node: split would make no improvement
				}
				CAGE_ASSERT_RUNTIME(bestAxis < 3);
				CAGE_ASSERT_RUNTIME(bestSplit + 1 < binsCount); // splits count is one less than bins count
				sortHelpersByAxis(indices.begin() + node.a, indices.begin() + (node.a + node.b), bestAxis);
				uint32 split = splitItems(bestSplit, node.b);
				sint32 leftNodeIndex = numeric_cast<sint32>(nodes.size());
				nodes.emplace_back(node.a, split);
				rebuild(leftNodeIndex, nodeDepth + 1, bestSah);
				sint32 rightNodeIndex = numeric_cast<sint32>(nodes.size());
				nodes.emplace_back(node.a + split, node.b - split);
				rebuild(rightNodeIndex, nodeDepth + 1, bestSah);
				node.a = -leftNodeIndex;
				node.b = -rightNodeIndex;
				node.box = nodes[leftNodeIndex].box + nodes[rightNodeIndex].box;
			}

			bool similar(const aabb &a, const aabb &b)
			{
				return (length(a.a - b.a) + length(a.b - b.b)) < 1e-3;
			}

			void validate(uint32 nodeIndex)
			{
				nodeStruct &node = nodes[nodeIndex];
				CAGE_ASSERT_RUNTIME((node.a < 0) == (node.b < 0));
				if (node.a < 0)
				{ // inner node
					nodeStruct &l = nodes[-node.a];
					nodeStruct &r = nodes[-node.b];
					CAGE_ASSERT_RUNTIME(similar(node.box, l.box + r.box));
					validate(-node.a);
					validate(-node.b);
				}
				else
				{ // leaf node
					aabb box;
					for (uint32 i = node.a, e = node.a + node.b; i < e; i++)
						box += indices[i]->box;
					CAGE_ASSERT_RUNTIME(similar(node.box, box));
				}
			}

			void rebuild()
			{
				dirty = true;
				nodes.clear();
				indices.clear();
				if (itemsTable->count() == 0)
				{
					dirty = false;
					return;
				}
				nodes.reserve(itemsTable->count());
				indices.reserve(itemsTable->count());
				for (const hashTablePair<itemBase> &it : *itemsTable)
					indices.push_back(it.second);
				nodes.emplace_back(0, numeric_cast<sint32>(itemsTable->count()));
				rebuild(0, 0, real::PositiveInfinity);
				CAGE_ASSERT_RUNTIME(nodes[0].box.valid());
#ifdef CAGE_DEBUG
				validate(0);
#endif // CAGE_DEBUG
				dirty = false;
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

			template<class T>
			struct intersectorStruct
			{
				const spatialDataImpl *data;
				std::vector<uint32> &resultNames;
				const T &other;

				intersectorStruct(const spatialDataImpl *data, std::vector<uint32> &resultNames, const T &other) : data(data), resultNames(resultNames), other(other)
				{
					intersection(0);
				}

				void intersection(uint32 nodeIndex)
				{
					const nodeStruct &node = data->nodes[nodeIndex];
					if (!intersects(other, node.box))
						return;
					if (node.a < 0)
					{ // internode
						intersection(-node.a);
						intersection(-node.b);
					}
					else
					{ // leaf
						for (uint32 i = node.a, e = node.a + node.b; i < e; i++)
						{
							itemBase *item = data->indices[i];
							if (item->intersects(other))
								resultNames.push_back(item->name);
						}
					}
				}
			};


			template<class T>
			void intersection(const T &other)
			{
				CAGE_ASSERT_RUNTIME(!data->dirty);
				clear();
				if (data->nodes.empty())
					return;
				intersectorStruct<T> i(data, resultNames, other);
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

	void spatialQueryClass::intersection(const plane &shape)
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
		impl->itemsTable->add(name, impl->itemsArena.createObject<itemShape<line>>(name, other));
	}

	void spatialDataClass::update(uint32 name, const triangle &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.area() < real::PositiveInfinity);
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->itemsTable->add(name, impl->itemsArena.createObject<itemShape<triangle>>(name, other));
	}

	void spatialDataClass::update(uint32 name, const sphere &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.volume() < real::PositiveInfinity);
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->itemsTable->add(name, impl->itemsArena.createObject<itemShape<sphere>>(name, other));
	}

	void spatialDataClass::update(uint32 name, const aabb &other)
	{
		CAGE_ASSERT_RUNTIME(other.valid());
		CAGE_ASSERT_RUNTIME(other.volume() < real::PositiveInfinity);
		spatialDataImpl *impl = (spatialDataImpl*)this;
		remove(name);
		impl->itemsTable->add(name, impl->itemsArena.createObject<itemShape<aabb>>(name, other));
	}

	void spatialDataClass::remove(uint32 name)
	{
		spatialDataImpl *impl = (spatialDataImpl*)this;
		impl->dirty = true;
		auto item = impl->itemsTable->get(name, true);
		if (!item)
			return;
		impl->itemsTable->remove(name);
		impl->itemsArena.deallocate(item);
	}

	void spatialDataClass::clear()
	{
		spatialDataImpl *impl = (spatialDataImpl*)this;
		impl->dirty = true;
		impl->itemsArena.flush();
		impl->itemsTable->clear();
	}

	void spatialDataClass::rebuild()
	{
		spatialDataImpl *impl = (spatialDataImpl*)this;
		impl->rebuild();
	}

	spatialDataCreateConfig::spatialDataCreateConfig() : maxItems(1000 * 100)
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
