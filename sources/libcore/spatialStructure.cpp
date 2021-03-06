#include <cage-core/geometry.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryArena.h>

#include <robin_hood.h>
#include <plf_colony.h>
#include <xsimd/xsimd.hpp>

#include <vector>
#include <algorithm>
#include <atomic>
#include <array>

namespace cage
{
	namespace
	{
		union alignas(16) FastPoint
		{
			xsimd::batch<float, 4> v4;
			struct
			{
				vec3 v3;
				union
				{
					real f;
					sint32 i;
				};
			} s;

			FastPoint() : v4{ 0,0,0,0 }
			{}
		};

		struct FastBox
		{
			FastPoint low;
			FastPoint high;

			// initialize to negative box
			FastBox()
			{
				low.v4 = xsimd::batch<float, 4>(real::Infinity().value, real::Infinity().value, real::Infinity().value, 0.0f);
				high.v4 = xsimd::batch<float, 4>(-real::Infinity().value, -real::Infinity().value, -real::Infinity().value, 0.0f);
			}

			explicit FastBox(const Aabb &b)
			{
				low.s.v3 = b.a;
				high.s.v3 = b.b;
			}

			FastBox operator + (const FastBox &other) const
			{
				FastBox r;
				r.low.v4 = xsimd::min(low.v4, other.low.v4);
				r.high.v4 = xsimd::max(high.v4, other.high.v4);
				return r;
			}

			FastBox &operator += (const FastBox &other)
			{
				return *this = *this + other;
			}

			real surface() const
			{
				if (empty())
					return 0;
				return Aabb(*this).surface();
			}

			bool empty() const
			{
				return low.v4[0] == real::Infinity();
			}

			explicit operator Aabb () const
			{
				return Aabb(low.s.v3, high.s.v3);
			}
		};

		bool intersects(const FastBox &a, const FastBox &b)
		{
			CAGE_ASSERT(uintPtr(&a) % alignof(FastBox) == 0);
			CAGE_ASSERT(uintPtr(&b) % alignof(FastBox) == 0);
			if (a.empty() || b.empty())
				return false;
			const xsimd::batch<float, 4> mask = { 1,1,1,0 };
			if (xsimd::any(a.high.v4 * mask < b.low.v4 * mask))
				return false;
			if (xsimd::any(a.low.v4 * mask > b.high.v4 * mask))
				return false;
			return true;
		}

		struct ItemBase
		{
			FastBox box;
			vec3 center;
			const uint32 name;
			
			virtual Aabb getBox() const = 0;
			virtual bool intersects(const Line &other) = 0;
			virtual bool intersects(const Triangle &other) = 0;
			virtual bool intersects(const Plane &other) = 0;
			virtual bool intersects(const Sphere &other) = 0;
			virtual bool intersects(const Aabb &other) = 0;
			virtual bool intersects(const Cone &other) = 0;
			virtual bool intersects(const Frustum &other) = 0;

			ItemBase(uint32 name) : name(name)
			{}

			void update()
			{
				Aabb b = getBox();
				box = FastBox(b);
				center = b.center();
			}
		};

		template<class T>
		struct ItemShape : public ItemBase, public T
		{
			ItemShape(uint32 name, const T &other) : ItemBase(name), T(other)
			{
				update();
			}

			virtual Aabb getBox() const { return Aabb(*(T*)(this)); }
			virtual bool intersects(const Line &other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Triangle &other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Plane &other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Sphere &other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Aabb &other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Cone &other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Frustum &other) { return cage::intersects(*(T *)this, other); };
		};

		struct Node
		{
			FastBox box;
			
			Node(const FastBox &box, sint32 a, sint32 b) : box(box)
			{
				this->a() = a;
				this->b() = b;
			}

			sint32 &a() { return box.low.s.i; } // negative -> inner node, -a = index of left child; non-negative -> leaf node, a = offset into itemNames array
			sint32 &b() { return box.high.s.i; } // negative -> inner node, -b = index of right child; non-negative -> leaf node, b = items count
			sint32 a() const { return box.low.s.i; }
			sint32 b() const { return box.high.s.i; }
		};

		struct ColonyAsAllocator
		{
			struct ItemAlloc
			{
				alignas(16) char reserved[128];
			};

			plf::colony<ItemAlloc, MemoryAllocatorStd<ItemAlloc>> colony;

			void *allocate(uintPtr size, uintPtr alignment)
			{
				CAGE_ASSERT(size <= sizeof(ItemAlloc));
				return &*colony.insert(ItemAlloc());
			}

			void deallocate(void *ptr)
			{
				colony.erase(colony.get_iterator_from_pointer((ItemAlloc *)ptr));
			}

			void flush()
			{
				CAGE_THROW_CRITICAL(Exception, "flush may not be used");
			}
		};

		class SpatialDataImpl : public SpatialStructure
		{
		public:
			ColonyAsAllocator itemsPool;
			MemoryArena itemsArena;
			robin_hood::unordered_map<uint32, Holder<ItemBase>> itemsTable;
			std::atomic<bool> dirty = false;
			std::vector<Node> nodes;
			std::vector<ItemBase *> indices;
			static constexpr uint32 binsCount = 10;
			std::array<FastBox, binsCount> leftBinBoxes = {};
			std::array<FastBox, binsCount> rightBinBoxes = {};
			std::array<uint32, binsCount> leftBinCounts = {};

			SpatialDataImpl(const SpatialStructureCreateConfig &config) : itemsArena(&itemsPool)
			{
				CAGE_ASSERT((uintPtr(this) % alignof(FastBox)) == 0);
				CAGE_ASSERT((uintPtr(leftBinBoxes.data()) % alignof(FastBox)) == 0);
				CAGE_ASSERT((uintPtr(rightBinBoxes.data()) % alignof(FastBox)) == 0);
			}

			~SpatialDataImpl()
			{
				clear();
			}

			void rebuild(uint32 nodeIndex, uint32 nodeDepth, real parentSah)
			{
				Node &node = nodes[nodeIndex];
				CAGE_ASSERT(node.a() >= 0 && node.b() >= 0); // is leaf now
				if (node.b() < 16)
					return; // leaf node: too few primitives
				uint32 bestAxis = m;
				uint32 bestSplit = m;
				uint32 bestItemsCount = 0;
				real bestSah = real::Infinity();
				FastBox bestBoxLeft;
				FastBox bestBoxRight;
				for (uint32 axis = 0; axis < 3; axis++)
				{
					if (node.box.high.v4[axis] - node.box.low.v4[axis] < 1e-7f)
						continue; // the box is flat (along this axis)
					for (FastBox &b : leftBinBoxes)
						b = FastBox();
					for (uint32 &c : leftBinCounts)
						c = 0;
					real binSizeInv = binsCount / (node.box.high.v4[axis] - node.box.low.v4[axis]);
					real planeOffset = node.box.low.v4[axis];
					for (sint32 i = node.a(), et = node.a() + node.b(); i != et; i++)
					{
						ItemBase *item = indices[i];
						uint32 binIndex = numeric_cast<uint32>((item->center[axis] - planeOffset) * binSizeInv);
						CAGE_ASSERT(binIndex <= binsCount);
						binIndex = min(binIndex, binsCount - 1);
						leftBinBoxes[binIndex] += item->box;
						leftBinCounts[binIndex]++;
					}
					// right to left sweep
					rightBinBoxes[binsCount - 1] = leftBinBoxes[binsCount - 1];
					for (uint32 i = binsCount - 1; i > 0; i--)
						rightBinBoxes[i - 1] = rightBinBoxes[i] + leftBinBoxes[i - 1];
					// left to right sweep
					for (uint32 i = 1; i < binsCount; i++)
					{
						leftBinBoxes[i] += leftBinBoxes[i - 1];
						leftBinCounts[i] += leftBinCounts[i - 1];
					}
					CAGE_ASSERT(leftBinCounts[binsCount - 1] == node.b());
					// compute sah
					for (uint32 i = 0; i < binsCount - 1; i++)
					{
						real sahL = leftBinBoxes[i].surface() * leftBinCounts[i];
						real sahR = rightBinBoxes[i + 1].surface() * (node.b() - leftBinCounts[i]);
						real sah = sahL + sahR;
						if (sah < bestSah)
						{
							bestAxis = axis;
							bestSplit = i;
							bestSah = sah;
							bestItemsCount = leftBinCounts[bestSplit];
							bestBoxLeft = leftBinBoxes[i];
							bestBoxRight = rightBinBoxes[i + 1];
						}
					}
				}
				CAGE_ASSERT(bestSah.valid());
				if (bestSah >= parentSah)
					return; // leaf node: split would make no improvement
				if (bestItemsCount == 0)
					return; // leaf node: split cannot separate any objects (they are probably all at one position)
				CAGE_ASSERT(bestAxis < 3);
				CAGE_ASSERT(bestSplit + 1 < binsCount); // splits count is one less than bins count
				CAGE_ASSERT(bestItemsCount < numeric_cast<uint32>(node.b()));
				{
					real binSizeInv = binsCount / (node.box.high.v4[bestAxis] - node.box.low.v4[bestAxis]);
					real planeOffset = node.box.low.v4[bestAxis];
					std::partition(indices.begin() + node.a(), indices.begin() + (node.a() + node.b()), [&](const ItemBase *item) {
						uint32 binIndex = numeric_cast<uint32>((item->center[bestAxis] - planeOffset) * binSizeInv);
						return binIndex < bestSplit + 1;
					});
				}
				sint32 leftNodeIndex = numeric_cast<sint32>(nodes.size());
				nodes.emplace_back(bestBoxLeft, node.a(), bestItemsCount);
				rebuild(leftNodeIndex, nodeDepth + 1, bestSah);
				sint32 rightNodeIndex = numeric_cast<sint32>(nodes.size());
				nodes.emplace_back(bestBoxRight, node.a() + bestItemsCount, node.b() - bestItemsCount);
				rebuild(rightNodeIndex, nodeDepth + 1, bestSah);
				node.a() = -leftNodeIndex;
				node.b() = -rightNodeIndex;
			}

			bool similar(const FastBox &a, const FastBox &b)
			{
				return (length(a.low.s.v3 - b.low.s.v3) + length(a.high.s.v3 - b.high.s.v3)) < 1e-3;
			}

			void validate(uint32 nodeIndex)
			{
				Node &node = nodes[nodeIndex];
				CAGE_ASSERT((node.a() < 0) == (node.b() < 0));
				if (node.a() < 0)
				{ // inner node
					Node &l = nodes[-node.a()];
					Node &r = nodes[-node.b()];
					validate(-node.a());
					validate(-node.b());
					CAGE_ASSERT(similar(node.box, l.box + r.box));
				}
				else
				{ // leaf node
					FastBox box;
					for (uint32 i = node.a(), e = node.a() + node.b(); i < e; i++)
						box += indices[i]->box;
					CAGE_ASSERT(similar(node.box, box));
				}
			}

			void rebuild()
			{
				dirty = true;
				nodes.clear();
				indices.clear();
				if (itemsTable.size() == 0)
				{
					dirty = false;
					return;
				}
				nodes.reserve(itemsTable.size());
				indices.reserve(itemsTable.size());
				FastBox worldBox;
				for (const auto &it : itemsTable)
				{
					indices.push_back(it.second.get());
					worldBox += it.second->box;
				}
				nodes.emplace_back(worldBox, 0, numeric_cast<sint32>(itemsTable.size()));
				rebuild(0, 0, real::Infinity());
				CAGE_ASSERT(uintPtr(nodes.data()) % alignof(Node) == 0);
#ifdef CAGE_ASSERT_ENABLED
				validate(0);
#endif // CAGE_ASSERT_ENABLED
				dirty = false;
			}
		};

		class SpatialQueryImpl : public SpatialQuery
		{
		public:
			const Holder<const SpatialDataImpl> data;
			std::vector<uint32> resultNames;

			SpatialQueryImpl(Holder<const SpatialDataImpl> data) : data(std::move(data))
			{
				resultNames.reserve(100);
			}

			void clear()
			{
				CAGE_ASSERT(!data->dirty);
				resultNames.clear();
			}

			template<class T>
			struct Intersector
			{
				const SpatialDataImpl *const data;
				std::vector<uint32> &resultNames;
				const T &other;
				const FastBox otherBox;

				Intersector(const SpatialDataImpl *data, std::vector<uint32> &resultNames, const T &other) : data(data), resultNames(resultNames), other(other), otherBox(Aabb(other))
				{
					CAGE_ASSERT((uintPtr(this) % alignof(FastBox)) == 0);
					CAGE_ASSERT((uintPtr(&otherBox) % alignof(FastBox)) == 0);
					intersection(0);
				}

				void intersection(uint32 nodeIndex)
				{
					const Node &node = data->nodes[nodeIndex];
					if (!intersects(otherBox, node.box))
						return;
					if (!intersects(other, Aabb(node.box)))
						return;
					if (node.a() < 0)
					{ // internode
						intersection(-node.a());
						intersection(-node.b());
					}
					else
					{ // leaf
						for (uint32 i = node.a(), e = node.a() + node.b(); i < e; i++)
						{
							ItemBase *item = data->indices[i];
							if (item->intersects(other))
								resultNames.push_back(item->name);
						}
					}
				}
			};

			template<class T>
			bool intersection(const T &other)
			{
				CAGE_ASSERT(!data->dirty);
				clear();
				if (data->nodes.empty())
					return false;
				Intersector<T> i(+data, resultNames, other);
				return !resultNames.empty();
			}
		};
	}

	PointerRange<uint32> SpatialQuery::result() const
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl*)this;
		return impl->resultNames;
	}

	bool SpatialQuery::intersection(const vec3 &shape)
	{
		return intersection(Aabb(shape, shape));
	}

	bool SpatialQuery::intersection(const Line &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl*)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Triangle &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl*)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Plane &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl*)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Sphere &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl*)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Aabb &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl*)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Cone &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Frustum &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	void SpatialStructure::update(uint32 name, const vec3 &other)
	{
		update(name, Aabb(other, other));
	}

	void SpatialStructure::update(uint32 name, const Line &other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.isPoint() || other.isSegment());
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		remove(name);
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Line>>(name, other);
	}

	void SpatialStructure::update(uint32 name, const Triangle &other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.area() < real::Infinity());
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		remove(name);
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Triangle>>(name, other);
	}

	void SpatialStructure::update(uint32 name, const Sphere &other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.volume() < real::Infinity());
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		remove(name);
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Sphere>>(name, other);
	}

	void SpatialStructure::update(uint32 name, const Aabb &other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.volume() < real::Infinity());
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		remove(name);
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Aabb>>(name, other);
	}

	void SpatialStructure::update(uint32 name, const Cone &other)
	{
		CAGE_ASSERT(other.valid());
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		remove(name);
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Cone>>(name, other);
	}

	void SpatialStructure::remove(uint32 name)
	{
		CAGE_ASSERT(name != m);
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		impl->dirty = true;
		impl->itemsTable.erase(name);
	}

	void SpatialStructure::clear()
	{
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		impl->dirty = true;
		impl->itemsTable.clear();
	}

	void SpatialStructure::rebuild()
	{
		SpatialDataImpl *impl = (SpatialDataImpl*)this;
		impl->rebuild();
	}

	Holder<SpatialStructure> newSpatialStructure(const SpatialStructureCreateConfig &config)
	{
		return systemMemory().createImpl<SpatialStructure, SpatialDataImpl>(config);
	}

	Holder<SpatialQuery> newSpatialQuery(Holder<const SpatialStructure> data)
	{
		return systemMemory().createImpl<SpatialQuery, SpatialQueryImpl>(std::move(data).cast<const SpatialDataImpl>());
	}
}
