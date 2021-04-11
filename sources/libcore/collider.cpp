#include <cage-core/geometry.h>
#include <cage-core/collider.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/mesh.h>

#include <vector>
#include <algorithm>

namespace cage
{
	namespace
	{
		class ColliderImpl : public Collider
		{
		public:
			struct Node
			{
				// for leaf - index of first triangle in this node
				// for node - -1
				uint32 left;

				// for leaf - index of one-after-last triangle in this node
				// for node - index of right child node
				uint32 right;

				Node() : left(m), right(m)
				{}
			};

			std::vector<Triangle> tris;
			std::vector<Aabb> boxes;
			std::vector<Node> nodes;
			bool dirty;

			ColliderImpl() : dirty(true)
			{
				((Collider *)this)->rebuild();
			}

			void build(std::vector<Triangle> &ts, uint32 level)
			{
				// find the total bounding box
				Aabb b;
				for (const auto &it : ts)
					b += Aabb(it);
				boxes.push_back(b);

				// primitive nodes form leaves
				if (ts.size() <= 10)
				{
					Node n;
					n.left = numeric_cast<uint32>(tris.size());
					n.right = n.left + numeric_cast<uint32>(ts.size());
					tris.insert(tris.end(), ts.begin(), ts.end());
					nodes.push_back(n);
					return;
				}

				// find partitioning axis and split plane
				uint32 axis = m;
				uint32 split = m;
				{
					real bestSah = real::Infinity();
					uint32 siz = numeric_cast<uint32>(ts.size());
					for (uint32 ax = 0; ax < 3; ax++)
					{
						if ((b.b[ax] - b.a[ax]) == 0)
							continue;
						constexpr uint32 planesCount = 12;
						Aabb interPlanes[planesCount + 1];
						uint32 counts[planesCount + 1];
						for (uint32 i = 0; i < planesCount + 1; i++)
							counts[i] = 0;
						real k1 = -b.a[ax];
						real k2 = (planesCount + 1) / (b.b[ax] - b.a[ax]);
						for (uint32 i = 0; i < siz; i++)
						{
							real c = ts[i].center()[ax];
							real d = (c + k1) * k2;
							d = clamp(d, 0, planesCount);
							uint32 j = numeric_cast<uint32>(d);
							CAGE_ASSERT(j < planesCount + 1);
							interPlanes[j] += Aabb(ts[i]);
							counts[j]++;
						}

						Aabb bl[planesCount];
						uint32 cl[planesCount];
						bl[0] = interPlanes[0];
						cl[0] = counts[0];
						for (uint32 i = 1; i < planesCount; i++)
						{
							bl[i] = bl[i - 1] + interPlanes[i];
							cl[i] = cl[i - 1] + counts[i];
						}
						Aabb br[planesCount];
						uint32 cr[planesCount];
						br[planesCount - 1] = interPlanes[planesCount];
						cr[planesCount - 1] = counts[planesCount];
						for (uint32 i = planesCount - 2; i != m; i--) // the iterator will overflow at the edge
						{
							br[i] = br[i + 1] + interPlanes[i + 1];
							cr[i] = cr[i + 1] + counts[i + 1];
						}

						for (uint32 i = 0; i < planesCount; i++)
						{
							real sah = bl[i].surface() * cl[i] + br[i].surface() * cr[i];
							if (sah < bestSah)
							{
								bestSah = sah;
								axis = ax;
								split = cl[i];
								CAGE_ASSERT(axis < 3);
								CAGE_ASSERT(split > 0 && split < ts.size());
							}
						}
					}
				}

				// actually split the triangles
				CAGE_ASSERT(axis < 3);
				CAGE_ASSERT(split > 0 && split < ts.size());
				std::sort(ts.begin(), ts.end(), [axis](const Triangle &a, const Triangle &b) {
					return a.center()[axis] < b.center()[axis];
				});
				std::vector<Triangle> left(ts.begin(), ts.begin() + split);
				std::vector<Triangle> right(ts.begin() + split, ts.end());
				// std::vector<Triangle>().swap(ts); // free some memory

				// build the node and recurse
				uint32 idx = numeric_cast<uint32>(nodes.size());
				nodes.push_back(Node());
				build(left, level + 1);
				nodes[idx].right = numeric_cast<uint32>(nodes.size());
				build(right, level + 1);
			}

			void validate(uint32 idx)
			{
				CAGE_ASSERT(idx < nodes.size());
				const Node &n = nodes[idx];
				if (n.left == m)
				{ // node
					CAGE_ASSERT(intersection(boxes[idx], boxes[idx + 1]) == boxes[idx + 1]);
					CAGE_ASSERT(intersection(boxes[idx], boxes[n.right]) == boxes[n.right]);
					validate(idx + 1);
					validate(n.right);
				}
				else
				{ // leaf
					for (uint32 i = n.left; i < n.right; i++)
					{
						const Triangle &t = tris[i];
						CAGE_ASSERT(intersection(boxes[idx], Aabb(t)) == Aabb(t));
					}
				}
			}

			void rebuild()
			{
				boxes.clear();
				nodes.clear();
				uint32 trisCount = numeric_cast<uint32>(tris.size());
				std::vector<Triangle> ts;
				ts.swap(tris);
				build(ts, 0);
				CAGE_ASSERT(tris.size() == trisCount);
				CAGE_ASSERT(boxes.size() == nodes.size());
#ifdef CAGE_DEBUG
				validate(0);
#endif // CAGE_DEBUG
			}
		};
	}

	PointerRange<const Triangle> Collider::triangles() const
	{
		const ColliderImpl *impl = (const ColliderImpl *)this;
		return impl->tris;
	}

	void Collider::addTriangle(const Triangle &t)
	{
		ColliderImpl *impl = (ColliderImpl *)this;
		impl->tris.push_back(t);
		impl->dirty = true;
	}

	void Collider::addTriangles(PointerRange<const Triangle> tris)
	{
		ColliderImpl *impl = (ColliderImpl *)this;
		impl->tris.insert(impl->tris.end(), tris.begin(), tris.end());
		impl->dirty = true;
	}

	void Collider::clear()
	{
		ColliderImpl *impl = (ColliderImpl *)this;
		impl->tris.clear();
		impl->dirty = true;
	}

	Holder<Collider> Collider::copy() const
	{
		Holder<Collider> res = newCollider();
		res->deserialize(serialize());
		return res;
	}

	void Collider::rebuild()
	{
		ColliderImpl *impl = (ColliderImpl *)this;
		if (!impl->dirty)
			return;
		impl->dirty = false;
		impl->rebuild();
	}

	bool Collider::needsRebuild() const
	{
		const ColliderImpl *impl = (const ColliderImpl *)this;
		return impl->dirty;
	}

	const Aabb &Collider::box() const
	{
		CAGE_ASSERT(!needsRebuild());
		const ColliderImpl *impl = (const ColliderImpl *)this;
		return impl->boxes[0];
	}

	namespace
	{
		constexpr uint16 currentVersion = 2;
		constexpr char currentMagic[] = "colid";

		struct CollisionModelHeader
		{
			char magic[6]; // colid\0
			uint16 version;
			uint32 trisCount;
			uint32 nodesCount;
			bool dirty;
		};

		template<class T>
		void writeVector(Serializer &ser, const std::vector<T> &v)
		{
			ser.write(bufferCast<const char, const T>(v));
		}

		template<class T>
		void readVector(Deserializer &des, std::vector<T> &v, uint32 count)
		{
			v.resize(count);
			des.read(bufferCast<char, T>(v));
		}
	}

	Holder<PointerRange<char>> Collider::serialize(bool includeAdditionalData) const
	{
		const ColliderImpl *impl = (const ColliderImpl *)this;
		CollisionModelHeader header;
		detail::memset(&header, 0, sizeof(header));
		detail::memcpy(header.magic, currentMagic, sizeof(currentMagic));
		header.version = currentVersion;
		header.trisCount = numeric_cast<uint32>(impl->tris.size());
		header.nodesCount = numeric_cast<uint32>(impl->nodes.size());
		header.dirty = impl->dirty;
		MemoryBuffer buffer;
		Serializer ser(buffer);
		ser << header;
		writeVector(ser, impl->tris);
		writeVector(ser, impl->boxes);
		writeVector(ser, impl->nodes);
		return templates::move(buffer);
	}

	void Collider::deserialize(PointerRange<const char> buffer)
	{
		ColliderImpl *impl = (ColliderImpl *)this;
		Deserializer des(buffer);
		CollisionModelHeader header;
		des >> header;
		if (detail::memcmp(header.magic, currentMagic, sizeof(currentMagic)) != 0)
			CAGE_THROW_ERROR(Exception, "Cannot deserialize collision object: wrong magic");
		if (header.version != currentVersion)
			CAGE_THROW_ERROR(Exception, "Cannot deserialize collision object: wrong version");
		impl->dirty = header.dirty;
		readVector(des, impl->tris, header.trisCount);
		readVector(des, impl->boxes, header.nodesCount);
		readVector(des, impl->nodes, header.nodesCount);
	}

	void Collider::importMesh(const Mesh *poly)
	{
		clear();
		CAGE_ASSERT(poly->type() == MeshTypeEnum::Triangles);
		if (poly->indices().empty())
		{
			CAGE_ASSERT(poly->positions().size() % 3 == 0);
			const uint32 cnt = poly->verticesCount() / 3;
			CAGE_ASSERT(sizeof(Triangle) == 3 * sizeof(vec3));
			addTriangles(bufferCast<const Triangle>(poly->positions()));
		}
		else
		{
			CAGE_ASSERT(poly->indices().size() % 3 == 0);
			const uint32 cnt = numeric_cast<uint32>(poly->indices().size()) / 3;
			const auto pos = poly->positions();
			const auto ind = poly->indices();
			for (uint32 i = 0; i < cnt; i++)
			{
				Triangle t;
				t[0] = pos[ind[i * 3 + 0]];
				t[1] = pos[ind[i * 3 + 1]];
				t[2] = pos[ind[i * 3 + 2]];
				addTriangle(t);
			}
		}
	}

	Holder<Collider> newCollider()
	{
		return systemArena().createImpl<Collider, ColliderImpl>();
	}

	Holder<Collider> newCollider(const MemoryBuffer &buffer)
	{
		Holder<Collider> tmp = newCollider();
		tmp->deserialize(buffer);
		return tmp;
	}

	bool operator < (const CollisionPair &l, const CollisionPair &r)
	{
		if (l.a == r.a)
			return l.b < r.b;
		return l.a < r.a;
	}

	namespace
	{
		template<class T, bool Transform>
		class LazyData {};

		template<class T>
		class LazyData<T, true>
		{
		public:
			std::vector<T> data;
			std::vector<bool> flags;
			const transform m;
			const T *const original;

			LazyData(const T *original, uint32 count, const transform &m) : m(m), original(original)
			{
				data.resize(count);
				flags.resize(count, false);
			}

			const T &operator[] (uint32 idx)
			{
				CAGE_ASSERT(idx < data.size());
				if (!flags[idx])
				{
					data[idx] = original[idx] * m;
					flags[idx] = true;
				}
				return data[idx];
			}
		};

		template<class T>
		class LazyData<T, false>
		{
		public:
			const T *const original;

			LazyData(const T *original, uint32 count, const transform &m) : original(original)
			{
				CAGE_ASSERT(m == transform());
			}

			const T &operator[] (uint32 idx) const
			{
				return original[idx];
			}
		};

		template<bool Swap>
		class CollisionDetector
		{
		public:
			LazyData<Triangle, Swap> ats;
			LazyData<Triangle, !Swap> bts;
			LazyData<Aabb, Swap> abs;
			LazyData<Aabb, !Swap> bbs;

			const ColliderImpl *const ao;
			const ColliderImpl *const bo;
			Holder<PointerRange<CollisionPair>> &outputBuffer;
			PointerRangeHolder<CollisionPair> collisions;

			CollisionDetector(const ColliderImpl *ao, const ColliderImpl *bo, const transform &am, const transform &bm, Holder<PointerRange<CollisionPair>> &outputBuffer) :
				ats(ao->tris.data(), numeric_cast<uint32>(ao->tris.size()), am), bts(bo->tris.data(), numeric_cast<uint32>(bo->tris.size()), bm),
				abs(ao->boxes.data(), numeric_cast<uint32>(ao->boxes.size()), am), bbs(bo->boxes.data(), numeric_cast<uint32>(bo->boxes.size()), bm),
				ao(ao), bo(bo), outputBuffer(outputBuffer)
			{}

			void process(uint32 a, uint32 b)
			{
				CAGE_ASSERT(a < ao->nodes.size());
				CAGE_ASSERT(b < bo->nodes.size());

				if (!intersects(abs[a], bbs[b]))
					return;

				const ColliderImpl::Node &an = ao->nodes[a];
				const ColliderImpl::Node &bn = bo->nodes[b];

				if (an.left != m && bn.left != m)
				{
					for (uint32 ai = an.left; ai < an.right; ai++)
					{
						const Triangle &at = ats[ai];
						for (uint32 bi = bn.left; bi < bn.right; bi++)
						{
							const Triangle &bt = bts[bi];
							if (intersects(at, bt))
							{
								CollisionPair p;
								p.a = ai;
								p.b = bi;
								collisions.push_back(p);
							}
						}
					}
					return;
				}

				if (an.left == m && bn.left == m)
				{
					process(a + 1, b + 1);
					process(a + 1, bn.right);
					process(an.right, b + 1);
					process(an.right, bn.right);
					return;
				}

				if (an.left == m)
				{
					process(a + 1, b);
					process(an.right, b);
				}

				if (bn.left == m)
				{
					process(a, b + 1);
					process(a, bn.right);
				}
			}

			void process()
			{
				process(0, 0);
				outputBuffer = templates::move(collisions);
			}
		};

		real timeOfContact(const Collider *ao, const Collider *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2)
		{
			return 0;
			/*
			real aRadius = ao->box().diagonal() * at1.scale * 0.5;
			real bRadius = bo->box().diagonal() * bt1.scale * 0.5;
			real radius = aRadius + bRadius;
			Line s = makeSegment(bt1.position - at1.position, bt2.position - at2.position);
			if (s.isPoint())
				return 0;
			Line r = intersection(s, Sphere(vec3(), radius));
			if (r.valid())
				return r.minimum / distance(bt1.position - at1.position, bt2.position - at2.position);
			else
				return real::Nan(); // no contact
			*/
		}

		real minSizeObject(const Collider *o, real scale)
		{
			vec3 s = o->box().size() * scale;
			return min(min(s[0], s[1]), s[2]);
		}

		real dist(const Line &l, const vec3 &p)
		{
			CAGE_ASSERT(l.normalized());
			return dot(p - l.origin, l.direction);
		}

		class IntersectionDetector
		{
		public:
			const ColliderImpl *const col;
			const transform m;

			IntersectionDetector(const ColliderImpl *collider, const transform &m) : col(collider), m(m)
			{}

			template<class T>
			real distance(const T &l, uint32 nodeIdx)
			{
				Aabb b = col->boxes[nodeIdx];
				if (!cage::intersects(l, b))
					return real::Nan();
				const auto &n = col->nodes[nodeIdx];
				if (n.left == cage::m)
				{ // node
					real c1 = distance(l, nodeIdx + 1);
					real c2 = distance(l, n.right);
					if (c1.valid())
					{
						if (c2.valid())
							return min(c1, c2);
						return c1;
					}
					return c2;
				}
				else
				{ // leaf
					real d = real::Infinity();
					for (uint32 ti = n.left; ti != n.right; ti++)
					{
						const Triangle &t = col->tris[ti];
						real p = cage::distance(l, t);
						if (p.valid() && p < d)
							d = p;
					}
					return d == real::Infinity() ? real::Nan() : d;
				}
			}

			template<class T>
			real distance(const T &shape)
			{
				return distance(shape * inverse(m), 0);
			}

			template<class T>
			bool intersects(const T &l, uint32 nodeIdx)
			{
				Aabb b = col->boxes[nodeIdx];
				if (!cage::intersects(l, b))
					return false;
				const auto &n = col->nodes[nodeIdx];
				if (n.left == cage::m)
				{ // node
					return intersects(l, nodeIdx + 1) || intersects(l, n.right);
				}
				else
				{ // leaf
					for (uint32 ti = n.left; ti != n.right; ti++)
					{
						const Triangle &t = col->tris[ti];
						if (cage::intersects(l, t))
							return true;
					}
					return false;
				}
			}

			template<class T>
			bool intersects(const T &shape)
			{
				return intersects(shape * inverse(m), 0);
			}

			bool intersection(const Line &l, uint32 nodeIdx, vec3 &point, uint32 &triangleIndex)
			{
				const Aabb b = col->boxes[nodeIdx];
				if (!cage::intersects(l, b))
					return false;
				const auto &n = col->nodes[nodeIdx];
				if (n.left == cage::m)
				{ // node
					vec3 c1, c2;
					uint32 t1 = 0, t2 = 0;
					const bool r1 = intersection(l, nodeIdx + 1, c1, t1);
					const bool r2 = intersection(l, n.right, c2, t2);
					if (r1)
					{
						if (r2)
						{
							if (dist(l, c1) < dist(l, c2))
							{
								point = c1;
								triangleIndex = t1;
								return true;
							}
							else
							{
								point = c2;
								triangleIndex = t2;
								return true;
							}
						}
						else
						{
							point = c1;
							triangleIndex = t1;
							return true;
						}
					}
					else if (r2)
					{
						point = c2;
						triangleIndex = t2;
						return true;
					}
					return false;
				}
				else
				{ // leaf
					real d = real::Infinity();
					bool res = false;
					for (uint32 ti = n.left; ti != n.right; ti++)
					{
						const Triangle &t = col->tris[ti];
						const vec3 p = cage::intersection(l, t);
						if (p.valid())
						{
							const real d1 = dist(l, p);
							if (d1 < d)
							{
								point = p;
								triangleIndex = ti;
								d = d1;
							}
							res = true;
						}
					}
					return res;
				}
			}

			bool intersection(const Line &l, vec3 &point, uint32 &triangleIndex)
			{
				vec3 pt;
				uint32 ti = 0;
				if (intersection(l * inverse(m), 0, pt, ti))
				{
					const vec4 r4 = vec4(pt, 1) * mat4(m);
					point = vec3(r4) / r4[3];
					triangleIndex = ti;
					return true;
				}
				return false;
			}
		};
	}



	bool collisionDetection(CollisionDetectionParams &params)
	{
		const ColliderImpl *const ao = (const ColliderImpl *)params.ao;
		const ColliderImpl *const bo = (const ColliderImpl *)params.bo;
		const transform &at1 = params.at1;
		const transform &bt1 = params.bt1;
		const transform &at2 = params.at2;
		const transform &bt2 = params.bt2;
		real &fractionBefore = params.fractionBefore;
		real &fractionContact = params.fractionContact;
		Holder<PointerRange<CollisionPair>> &outputBuffer = params.collisionPairs;

		CAGE_ASSERT(!ao->needsRebuild());
		CAGE_ASSERT(!bo->needsRebuild());

		if (at1 == at2 && bt1 == bt2)
		{
			fractionBefore = 0;
			fractionContact = 0;
			if (ao->triangles().size() > bo->triangles().size())
			{
				CollisionDetector<false> d(ao, bo, transform(), transform(inverse(at1) * bt1), outputBuffer);
				d.process();
				return !outputBuffer.empty();
			}
			else
			{
				CollisionDetector<true> d(ao, bo, transform(inverse(bt1) * at1), transform(), outputBuffer);
				d.process();
				return !outputBuffer.empty();
			}
		}
		else
		{
			CAGE_ASSERT(at1.scale == at2.scale);
			CAGE_ASSERT(bt1.scale == bt2.scale);

			// find approximate time range of contact
			real time1 = timeOfContact(ao, bo, at1, bt1, at2, bt2);
			if (!time1.valid())
				return false;
			CAGE_ASSERT(time1 >= 0 && time1 <= 1);
			real time2 = 1 - timeOfContact(ao, bo, at2, bt2, at1, bt1);
			CAGE_ASSERT(time2.valid());
			CAGE_ASSERT(time2 >= 0 && time2 <= 1);
			CAGE_ASSERT(time2 >= time1);

			// find first contact
			const real minSize = min(minSizeObject(ao, at1.scale), minSizeObject(bo, bt1.scale)) * 0.5;
			const real maxDist = max(distance(interpolate(at1.position, at2.position, time1), interpolate(at1.position, at2.position, time2)),
				distance(interpolate(bt1.position, bt2.position, time1), interpolate(bt1.position, bt2.position, time2)));
			real maxDiff = (maxDist > minSize ? (minSize / maxDist) : 1) * (time2 - time1);
			CAGE_ASSERT(maxDiff > 0 && maxDiff <= 1);
			maxDiff = min(maxDiff, (time2 - time1) * 0.2);
			while (time1 <= time2)
			{
				CollisionDetectionParams p(ao, bo, interpolate(at1, at2, time1), interpolate(bt1, bt2, time1));
				if (collisionDetection(p))
				{
					time2 = time1;
					time1 = max(time1 - maxDiff, 0);
					break;
				}
				else
					time1 += maxDiff;
			}
			if (time1 > time2)
				return false;

			// improve collision precision by binary search
			fractionBefore = time1;
			fractionContact = time2;
			for (uint32 i = 0; i < 6; i++)
			{
				const real time = (time1 + time2) * 0.5;
				CollisionDetectionParams p(ao, bo, interpolate(at1, at2, time), interpolate(bt1, bt2, time));
				if (collisionDetection(p))
				{
					time2 = time;
					fractionContact = time;
				}
				else
				{
					time1 = time;
					fractionBefore = time;
				}
			}
			CAGE_ASSERT(fractionBefore >= 0 && fractionBefore <= 1);
			CAGE_ASSERT(fractionContact >= 0 && fractionContact <= 1);
			CAGE_ASSERT(fractionBefore <= fractionContact);

#ifdef CAGE_ASSERT_ENABLED
			{
				// verify that there is no collision at fractionBefore
				CollisionDetectionParams p(ao, bo, interpolate(at1, at2, fractionBefore), interpolate(bt1, bt2, fractionBefore));
				CAGE_ASSERT(fractionBefore == 0 || !collisionDetection(p));
			}
#endif

			{
				// find the actual collision at fractionContact
				CollisionDetectionParams p(ao, bo, interpolate(at1, at2, fractionContact), interpolate(bt1, bt2, fractionContact));
				bool res = collisionDetection(p);
				CAGE_ASSERT(res);
				outputBuffer = templates::move(p.collisionPairs);
			}
			return true;
		}
	}



	real distance(const Line &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.distance(shape);
	}

	real distance(const Triangle &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.distance(shape);
	}

	real distance(const Plane &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.distance(shape);
	}

	real distance(const Sphere &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.distance(shape);
	}

	real distance(const Aabb &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.distance(shape);
	}

	real distance(const Collider *ao, const Collider *bo, const transform &at, const transform &bt)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "Collider-Collider distance");
	}



	bool intersects(const Line &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const Triangle &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const Plane &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const Sphere &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const Aabb &shape, const Collider *collider, const transform &t)
	{
		IntersectionDetector d((const ColliderImpl *)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const Collider *ao, const Collider *bo, const transform &at, const transform &bt)
	{
		CollisionDetectionParams p(ao, bo, at, bt);
		return collisionDetection(p);
	}


	vec3 intersection(const Line &shape, const Collider *collider, const transform &t)
	{
		uint32 triangleIndex = m;
		return intersection(shape, collider, t, triangleIndex);
	}

	vec3 intersection(const Line &shape, const Collider *collider, const transform &t, uint32 &triangleIndex)
	{
		// todo
		IntersectionDetector d((const ColliderImpl *)collider, t);
		vec3 p;
		if (d.intersection(shape, p, triangleIndex))
			return p;
		return vec3::Nan();
	}
}
