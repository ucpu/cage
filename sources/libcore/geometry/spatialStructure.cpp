#include <algorithm>
#include <array>
#include <atomic>
#include <vector>

#include <unordered_dense.h>

#include <cage-core/geometry.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/spatialStructure.h>

namespace cage
{
	namespace
	{
		union alignas(16) FastPoint
		{
			Vec4 v4;
			struct
			{
				Vec3 v3;
				union
				{
					Real f;
					sint32 i;
				};
			} s;

			CAGE_FORCE_INLINE FastPoint() : v4() {}
		};

		struct FastBox
		{
			FastPoint low;
			FastPoint high;

			// initialize to negative box
			CAGE_FORCE_INLINE FastBox()
			{
				low.v4 = Vec4(Vec3(Real::Infinity()), 0.0f);
				high.v4 = Vec4(Vec3(-Real::Infinity()), 0.0f);
			}

			CAGE_FORCE_INLINE explicit FastBox(Aabb b)
			{
				low.s.v3 = b.a;
				high.s.v3 = b.b;
			}

			CAGE_FORCE_INLINE FastBox operator+(FastBox other) const
			{
				FastBox r;
				r.low.v4 = min(low.v4, other.low.v4);
				r.high.v4 = max(high.v4, other.high.v4);
				return r;
			}

			CAGE_FORCE_INLINE FastBox &operator+=(FastBox other) { return *this = *this + other; }

			CAGE_FORCE_INLINE Real surface() const
			{
				if (empty())
					return 0;
				return Aabb(*this).surface();
			}

			CAGE_FORCE_INLINE bool empty() const { return low.v4[0] == Real::Infinity(); }

			CAGE_FORCE_INLINE explicit operator Aabb() const
			{
				Aabb r; // using the constructor calls unnecessary min/max
				r.a = low.s.v3;
				r.b = high.s.v3;
				return r;
			}
		};

		CAGE_FORCE_INLINE bool intersects(FastBox a, FastBox b)
		{
			CAGE_ASSERT(uintPtr(&a) % alignof(FastBox) == 0);
			CAGE_ASSERT(uintPtr(&b) % alignof(FastBox) == 0);
			if (a.empty() || b.empty())
				return false;
			// bitwise OR to avoid branching
			return !(int(a.high.v4[0] < b.low.v4[0]) | int(a.high.v4[1] < b.low.v4[1]) | int(a.high.v4[2] < b.low.v4[2]) | int(a.low.v4[0] > b.high.v4[0]) | int(a.low.v4[1] > b.high.v4[1]) | int(a.low.v4[2] > b.high.v4[2]));
		}

		struct ItemBase
		{
			FastBox box;
			Vec3 center;
			const uint32 name;

			virtual Aabb getBox() const = 0;
			virtual bool intersects(Line other) = 0;
			virtual bool intersects(Triangle other) = 0;
			virtual bool intersects(Plane other) = 0;
			virtual bool intersects(Sphere other) = 0;
			virtual bool intersects(Aabb other) = 0;
			virtual bool intersects(Cone other) = 0;
			virtual bool intersects(const Frustum &other) = 0;

			CAGE_FORCE_INLINE explicit ItemBase(uint32 name) : name(name) {}

			CAGE_FORCE_INLINE void update()
			{
				const Aabb b = getBox();
				box = FastBox(b);
				center = b.center();
			}
		};

		template<class T>
		struct ItemShape : public ItemBase, public T
		{
			CAGE_FORCE_INLINE ItemShape(uint32 name, T other) : ItemBase(name), T(other) { update(); }

			virtual Aabb getBox() const { return Aabb(*(T *)this); }
			virtual bool intersects(Line other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(Triangle other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(Plane other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(Sphere other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(Aabb other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(Cone other) { return cage::intersects(*(T *)this, other); };
			virtual bool intersects(const Frustum &other) { return cage::intersects(*(T *)this, other); };
		};

		struct Node
		{
			FastBox box;

			CAGE_FORCE_INLINE Node(FastBox box, sint32 a, sint32 b) : box(box)
			{
				this->a() = a;
				this->b() = b;
			}

			CAGE_FORCE_INLINE sint32 &a() { return box.low.s.i; } // negative -> inner node, -a = index of left child; non-negative -> leaf node, a = offset into itemNames array
			CAGE_FORCE_INLINE sint32 &b() { return box.high.s.i; } // negative -> inner node, -b = index of right child; non-negative -> leaf node, b = items count
			CAGE_FORCE_INLINE sint32 a() const { return box.low.s.i; }
			CAGE_FORCE_INLINE sint32 b() const { return box.high.s.i; }
		};

		class SpatialDataImpl : public SpatialStructure
		{
		public:
			Holder<MemoryArena> itemsPool = newMemoryAllocatorPool({ 144, 16 });
			MemoryArena itemsArena;
			ankerl::unordered_dense::map<uint32, Holder<ItemBase>> itemsTable;
			std::atomic<bool> dirty = false;
			std::vector<Node> nodes;
			std::vector<ItemBase *> indices;
			static constexpr uint32 BinsCount = 10;
			std::array<FastBox, BinsCount> leftBinBoxes = {};
			std::array<FastBox, BinsCount> rightBinBoxes = {};
			std::array<uint32, BinsCount> leftBinCounts = {};

			SpatialDataImpl(const SpatialStructureCreateConfig &config) : itemsArena(*itemsPool)
			{
				CAGE_ASSERT((uintPtr(this) % alignof(FastBox)) == 0);
				CAGE_ASSERT((uintPtr(leftBinBoxes.data()) % alignof(FastBox)) == 0);
				CAGE_ASSERT((uintPtr(rightBinBoxes.data()) % alignof(FastBox)) == 0);
				itemsTable.reserve(config.reserve);
			}

			~SpatialDataImpl() { clear(); }

			void rebuild(uint32 nodeIndex, uint32 nodeDepth, Real parentSah)
			{
				Node &node = nodes[nodeIndex];
				CAGE_ASSERT(node.a() >= 0 && node.b() >= 0); // is leaf now
				if (node.b() < 10)
					return; // leaf node: too few primitives
				uint32 bestAxis = m;
				uint32 bestSplit = m;
				uint32 bestItemsCount = 0;
				Real bestSah = Real::Infinity();
				FastBox bestBoxLeft;
				FastBox bestBoxRight;
				for (uint32 axis = 0; axis < 3; axis++)
				{
					if (node.box.high.v4[axis] - node.box.low.v4[axis] < 1e-7f)
						continue; // the box is flat (along this axis)
					leftBinBoxes = {};
					leftBinCounts = {};
					const Real binSizeInv = BinsCount / (node.box.high.v4[axis] - node.box.low.v4[axis]);
					const Real planeOffset = node.box.low.v4[axis];
					for (sint32 i = node.a(), et = node.a() + node.b(); i != et; i++)
					{
						ItemBase *item = indices[i];
						uint32 binIndex = numeric_cast<uint32>((item->center[axis] - planeOffset) * binSizeInv);
						CAGE_ASSERT(binIndex <= BinsCount);
						binIndex = min(binIndex, BinsCount - 1);
						leftBinBoxes[binIndex] += item->box;
						leftBinCounts[binIndex]++;
					}
					// right to left sweep
					rightBinBoxes[BinsCount - 1] = leftBinBoxes[BinsCount - 1];
					for (uint32 i = BinsCount - 1; i > 0; i--)
						rightBinBoxes[i - 1] = rightBinBoxes[i] + leftBinBoxes[i - 1];
					// left to right sweep
					for (uint32 i = 1; i < BinsCount; i++)
					{
						leftBinBoxes[i] += leftBinBoxes[i - 1];
						leftBinCounts[i] += leftBinCounts[i - 1];
					}
					CAGE_ASSERT(leftBinCounts[BinsCount - 1] == node.b());
					// compute sah
					for (uint32 i = 0; i < BinsCount - 1; i++)
					{
						const Real sahL = leftBinBoxes[i].surface() * leftBinCounts[i];
						const Real sahR = rightBinBoxes[i + 1].surface() * (node.b() - leftBinCounts[i]);
						const Real sah = sahL + sahR;
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
				CAGE_ASSERT(bestSplit + 1 < BinsCount); // splits count is one less than bins count
				CAGE_ASSERT(bestItemsCount < numeric_cast<uint32>(node.b()));
				{
					const Real binSizeInv = BinsCount / (node.box.high.v4[bestAxis] - node.box.low.v4[bestAxis]);
					const Real planeOffset = node.box.low.v4[bestAxis];
					std::partition(indices.begin() + node.a(), indices.begin() + (node.a() + node.b()),
						[&](const ItemBase *item)
						{
							const uint32 binIndex = numeric_cast<uint32>((item->center[bestAxis] - planeOffset) * binSizeInv);
							return binIndex < bestSplit + 1;
						});
				}
				const sint32 leftNodeIndex = numeric_cast<sint32>(nodes.size());
				nodes.emplace_back(bestBoxLeft, node.a(), bestItemsCount);
				rebuild(leftNodeIndex, nodeDepth + 1, bestSah);
				const sint32 rightNodeIndex = numeric_cast<sint32>(nodes.size());
				nodes.emplace_back(bestBoxRight, node.a() + bestItemsCount, node.b() - bestItemsCount);
				rebuild(rightNodeIndex, nodeDepth + 1, bestSah);
				node.a() = -leftNodeIndex;
				node.b() = -rightNodeIndex;
			}

			static bool similar(FastBox a, FastBox b) { return (length(a.low.s.v3 - b.low.s.v3) + length(a.high.s.v3 - b.high.s.v3)) < 1e-3; }

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
					indices.push_back(+it.second);
					worldBox += it.second->box;
				}
				nodes.emplace_back(worldBox, 0, numeric_cast<sint32>(itemsTable.size()));
				rebuild(0, 0, Real::Infinity());
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

			SpatialQueryImpl(Holder<const SpatialDataImpl> data) : data(std::move(data)) { resultNames.reserve(100); }

			CAGE_FORCE_INLINE void clear()
			{
				CAGE_ASSERT(!data->dirty);
				resultNames.clear();
			}

			template<class T>
			struct Intersector
			{
				static_assert(std::is_same_v<T, std::remove_const_t<T>>);
				const FastBox otherBox;
				const T other;
				const SpatialDataImpl *const data;
				std::vector<uint32> &resultNames;

				CAGE_FORCE_INLINE Intersector(const SpatialDataImpl *data, std::vector<uint32> &resultNames, const T &other) : otherBox(Aabb(other)), other(other), data(data), resultNames(resultNames)
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
					if constexpr (!std::is_same_v<T, Aabb>)
					{
						// this would have same result for aabb as the test above
						if (!intersects(other, Aabb(node.box)))
							return;
					}
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
			CAGE_FORCE_INLINE bool intersection(T other)
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
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->resultNames;
	}

	bool SpatialQuery::intersection(Vec3 shape)
	{
		return intersection(Aabb(shape, shape));
	}

	bool SpatialQuery::intersection(Line shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(Triangle shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(Plane shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(Sphere shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(Aabb shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(Cone shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	bool SpatialQuery::intersection(const Frustum &shape)
	{
		SpatialQueryImpl *impl = (SpatialQueryImpl *)this;
		return impl->intersection(shape);
	}

	void SpatialStructure::update(uint32 name, Vec3 other)
	{
		update(name, Aabb(other, other));
	}

	void SpatialStructure::update(uint32 name, Line other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.isPoint() || other.isSegment());
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Line>>(name, other);
	}

	void SpatialStructure::update(uint32 name, Triangle other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.area() < Real::Infinity());
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Triangle>>(name, other);
	}

	void SpatialStructure::update(uint32 name, Sphere other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.volume() < Real::Infinity());
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Sphere>>(name, other);
	}

	void SpatialStructure::update(uint32 name, Aabb other)
	{
		CAGE_ASSERT(other.valid());
		CAGE_ASSERT(other.volume() < Real::Infinity());
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Aabb>>(name, other);
	}

	void SpatialStructure::update(uint32 name, Cone other)
	{
		CAGE_ASSERT(other.valid());
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable[name] = impl->itemsArena.createImpl<ItemBase, ItemShape<Cone>>(name, other);
	}

	void SpatialStructure::remove(uint32 name)
	{
		CAGE_ASSERT(name != m);
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable.erase(name);
	}

	void SpatialStructure::clear()
	{
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->dirty = true;
		impl->itemsTable.clear();
	}

	void SpatialStructure::rebuild()
	{
		SpatialDataImpl *impl = (SpatialDataImpl *)this;
		impl->rebuild();
	}

	Holder<SpatialStructure> newSpatialStructure(const SpatialStructureCreateConfig &config)
	{
		return systemMemory().createImpl<SpatialStructure, SpatialDataImpl>(config);
	}

	Holder<SpatialQuery> newSpatialQuery(Holder<const SpatialStructure> data)
	{
		CAGE_ASSERT(data);
		return systemMemory().createImpl<SpatialQuery, SpatialQueryImpl>(std::move(data).cast<const SpatialDataImpl>());
	}
}
