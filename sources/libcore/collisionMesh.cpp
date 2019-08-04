#include <vector>
#include <algorithm>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/collisionMesh.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

namespace cage
{
	namespace
	{
		class collisionObjectImpl : public collisionMesh
		{
		public:
			struct nodeStruct
			{
				// for leaf - index of first triangle in this node
				// for node - -1
				uint32 left;

				// for leaf - index of one-after-last triangle in this node
				// for node - index of right child node
				uint32 right;

				nodeStruct() : left(m), right(m)
				{}
			};

			std::vector<triangle> tris;
			std::vector<aabb> boxes;
			std::vector<nodeStruct> nodes;
			bool dirty;

			collisionObjectImpl() : dirty(true)
			{
				((collisionMesh*)this)->rebuild();
			}

			void build(std::vector<triangle> &ts, uint32 level)
			{
				// find the total bounding box
				aabb b;
				for (auto &it : ts)
					b += aabb(it);
				boxes.push_back(b);

				// primitive nodes form a leaves
				if (ts.size() <= 10)
				{
					nodeStruct n;
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
						static const uint32 planesCount = 12;
						aabb boxes[planesCount + 1];
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
							CAGE_ASSERT_RUNTIME(j < planesCount + 1);
							boxes[j] += aabb(ts[i]);
							counts[j]++;
						}

						aabb bl[planesCount];
						uint32 cl[planesCount];
						bl[0] = boxes[0];
						cl[0] = counts[0];
						for (uint32 i = 1; i < planesCount; i++)
						{
							bl[i] = bl[i - 1] + boxes[i];
							cl[i] = cl[i - 1] + counts[i];
						}
						aabb br[planesCount];
						uint32 cr[planesCount];
						br[planesCount - 1] = boxes[planesCount];
						cr[planesCount - 1] = counts[planesCount];
						for (uint32 i = planesCount - 2; i < planesCount; i--) // the iterator will overflow at the edge
						{
							br[i] = br[i + 1] + boxes[i + 1];
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
								CAGE_ASSERT_RUNTIME(axis < 3, axis, split, ts.size());
								CAGE_ASSERT_RUNTIME(split > 0 && split < ts.size(), axis, split, ts.size());
							}
						}
					}
				}

				// actually split the triangles
				CAGE_ASSERT_RUNTIME(axis < 3, axis, split, ts.size());
				CAGE_ASSERT_RUNTIME(split > 0 && split < ts.size(), axis, split, ts.size());
				std::sort(ts.begin(), ts.end(), [axis](const triangle &a, const triangle &b) {
					return a.center()[axis] < b.center()[axis];
				});
				std::vector<triangle> left(ts.begin(), ts.begin() + split);
				std::vector<triangle> right(ts.begin() + split, ts.end());
				// std::vector<triangle>().swap(ts); // free some memory

				// build the node and recurse
				uint32 idx = numeric_cast<uint32>(nodes.size());
				nodes.push_back(nodeStruct());
				build(left, level + 1);
				nodes[idx].right = numeric_cast<uint32>(nodes.size());
				build(right, level + 1);
			}

			void validate(uint32 idx)
			{
				CAGE_ASSERT_RUNTIME(idx < nodes.size(), idx, nodes.size());
				const nodeStruct &n = nodes[idx];
				if (n.left == m)
				{ // node
					CAGE_ASSERT_RUNTIME(intersection(boxes[idx], boxes[idx + 1]) == boxes[idx + 1]);
					CAGE_ASSERT_RUNTIME(intersection(boxes[idx], boxes[n.right]) == boxes[n.right]);
					validate(idx + 1);
					validate(n.right);
				}
				else
				{ // leaf
					for (uint32 i = n.left; i < n.right; i++)
					{
						const triangle &t = tris[i];
						CAGE_ASSERT_RUNTIME(intersection(boxes[idx], aabb(t)) == aabb(t));
					}
				}
			}

			void rebuild()
			{
				boxes.clear();
				nodes.clear();
				uint32 trisCount = numeric_cast<uint32>(tris.size());
				std::vector<triangle> ts;
				ts.swap(tris);
				build(ts, 0);
				CAGE_ASSERT_RUNTIME(tris.size() == trisCount, tris.size(), trisCount);
				CAGE_ASSERT_RUNTIME(boxes.size() == nodes.size(), boxes.size(), nodes.size());
#ifdef CAGE_DEBUG
				//validate(0);
#endif // CAGE_DEBUG
			}
		};
	}

	uint32 collisionMesh::trianglesCount() const
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		return numeric_cast<uint32>(impl->tris.size());
	}

	const triangle *collisionMesh::trianglesData() const
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		return impl->tris.data();
	}

	const triangle &collisionMesh::triangleData(uint32 idx) const
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		return impl->tris[idx];
	}

	pointerRange<const triangle> collisionMesh::triangles() const
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		return impl->tris;
	}

	void collisionMesh::addTriangle(const triangle &t)
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		impl->tris.push_back(t);
		impl->dirty = true;
	}

	void collisionMesh::addTriangles(const triangle *data, uint32 count)
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		impl->tris.insert(impl->tris.end(), data, data + count);
		impl->dirty = true;
	}

	void collisionMesh::clear()
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		impl->tris.clear();
		impl->dirty = true;
	}

	void collisionMesh::rebuild()
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		if (!impl->dirty)
			return;
		impl->dirty = false;
		impl->rebuild();
	}

	bool collisionMesh::needsRebuild() const
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		return impl->dirty;
	}

	const aabb &collisionMesh::box() const
	{
		CAGE_ASSERT_RUNTIME(!needsRebuild());
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		return impl->boxes[0];
	}

	namespace
	{
		static const uint16 currentVersion = 2;
		static const char *const currentMagic = "colid";
		struct collisionObjectHeader
		{
			char magic[6]; // colid\0
			uint16 version;
			uint32 trisCount;
			uint32 nodesCount;
			bool dirty;
		};

		template<class T> void writeVector(serializer &ser, const std::vector<T> &v)
		{
			ser.write(v.data(), sizeof(T) * v.size());
		}

		template<class T> void readVector(deserializer &des, std::vector<T> &v, uint32 count)
		{
			v.resize(count);
			des.read(v.data(), sizeof(T) * count);
		}
	}

	memoryBuffer collisionMesh::serialize(bool includeAdditionalData) const
	{
		const collisionObjectImpl *impl = (collisionObjectImpl*)this;
		collisionObjectHeader header;
		detail::memset(&header, 0, sizeof(header));
		detail::strcpy(header.magic, currentMagic);
		header.version = currentVersion;
		header.trisCount = numeric_cast<uint32>(impl->tris.size());
		header.nodesCount = numeric_cast<uint32>(impl->nodes.size());
		header.dirty = impl->dirty;
		memoryBuffer buffer;
		serializer ser(buffer);
		ser << header;
		writeVector(ser, impl->tris);
		writeVector(ser, impl->boxes);
		writeVector(ser, impl->nodes);
		return buffer;
	}

	void collisionMesh::deserialize(const memoryBuffer &buffer)
	{
		collisionObjectImpl *impl = (collisionObjectImpl*)this;
		deserializer des(buffer);
		collisionObjectHeader header;
		des >> header;
		if (detail::memcmp(header.magic, currentMagic, detail::strlen(currentMagic)) != 0)
			CAGE_THROW_ERROR(exception, "Cannot deserialize collision object: wrong magic");
		if (header.version != currentVersion)
			CAGE_THROW_ERROR(exception, "Cannot deserialize collision object: wrong version");
		impl->dirty = header.dirty;
		readVector(des, impl->tris, header.trisCount);
		readVector(des, impl->boxes, header.nodesCount);
		readVector(des, impl->nodes, header.nodesCount);
	}

	holder<collisionMesh> newCollisionMesh()
	{
		return detail::systemArena().createImpl<collisionMesh, collisionObjectImpl>();
	}

	holder<collisionMesh> newCollisionMesh(const memoryBuffer &buffer)
	{
		holder<collisionMesh> tmp = newCollisionMesh();
		tmp->deserialize(buffer);
		return tmp;
	}

	collisionPair::collisionPair() : a(m), b(m)
	{}

	collisionPair::collisionPair(uint32 a, uint32 b) : a(a), b(b)
	{}

	bool collisionPair::operator < (const collisionPair &other) const
	{
		if (a == other.a)
			return b < other.b;
		return a < b;
	}

	namespace
	{
		template<class T, bool Transform>
		class lazyData {};

		template<class T>
		class lazyData<T, true>
		{
		public:
			std::vector<T> data;
			std::vector<bool> flags;
			const transform m;
			const T *const original;

			lazyData(const T *original, uint32 count, const transform &m) : m(m), original(original)
			{
				data.resize(count);
				flags.resize(count, false);
			}

			const T &operator[] (uint32 idx)
			{
				CAGE_ASSERT_RUNTIME(idx < data.size(), idx, data.size());
				if (!flags[idx])
				{
					data[idx] = original[idx] * m;
					flags[idx] = true;
				}
				return data[idx];
			}
		};

		template<class T>
		class lazyData<T, false>
		{
		public:
			const T *const original;

			lazyData(const T *original, uint32 count, const transform &m) : original(original)
			{
				CAGE_ASSERT_RUNTIME(m == transform());
			}

			const T &operator[] (uint32 idx) const
			{
				return original[idx];
			}
		};

		template<bool Swap>
		class collisionDetector
		{
		public:
			lazyData<triangle, Swap> ats;
			lazyData<triangle, !Swap> bts;
			lazyData<aabb, Swap> abs;
			lazyData<aabb, !Swap> bbs;

			const collisionObjectImpl *ao;
			const collisionObjectImpl *bo;
			collisionPair *outputBuffer;
			uint32 bufferSize;
			uint32 result;

			collisionDetector(const collisionObjectImpl *ao, const collisionObjectImpl *bo, const transform &am, const transform &bm, collisionPair *outputBuffer, uint32 bufferSize) :
				ats(ao->tris.data(), numeric_cast<uint32>(ao->tris.size()), am), bts(bo->tris.data(), numeric_cast<uint32>(bo->tris.size()), bm),
				abs(ao->boxes.data(), numeric_cast<uint32>(ao->boxes.size()), am), bbs(bo->boxes.data(), numeric_cast<uint32>(bo->boxes.size()), bm),
				ao(ao), bo(bo), outputBuffer(outputBuffer), bufferSize(bufferSize), result(0)
			{}

			void process(uint32 a, uint32 b)
			{
				CAGE_ASSERT_RUNTIME(a < ao->nodes.size(), a, ao->nodes.size());
				CAGE_ASSERT_RUNTIME(b < bo->nodes.size(), b, bo->nodes.size());

				if (!intersects(abs[a], bbs[b]) || result == bufferSize)
					return;

				const collisionObjectImpl::nodeStruct &an = ao->nodes[a];
				const collisionObjectImpl::nodeStruct &bn = bo->nodes[b];

				if (an.left != m && bn.left != m)
				{
					for (uint32 ai = an.left; ai < an.right; ai++)
					{
						const triangle &at = ats[ai];
						for (uint32 bi = bn.left; bi < bn.right; bi++)
						{
							const triangle &bt = bts[bi];
							if (intersects(at, bt))
							{
								auto &p = outputBuffer[result++];
								p.a = ai;
								p.b = bi;
								if (result == bufferSize)
									return;
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

			const uint32 process()
			{
				process(0, 0);
				return result;
			}
		};

		real timeOfContact(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2)
		{
			return 0;
			/*
			real aRadius = ao->box().diagonal() * at1.scale * 0.5;
			real bRadius = bo->box().diagonal() * bt1.scale * 0.5;
			real radius = aRadius + bRadius;
			line s = makeSegment(bt1.position - at1.position, bt2.position - at2.position);
			if (s.isPoint())
				return 0;
			line r = intersection(s, sphere(vec3(), radius));
			if (r.valid())
				return r.minimum / distance(bt1.position - at1.position, bt2.position - at2.position);
			else
				return real::Nan(); // no contact
			*/
		}

		real minSizeObject(const collisionMesh *o, real scale)
		{
			vec3 s = o->box().size() * scale;
			return min(min(s[0], s[1]), s[2]);
		}

		real dist(const line &l, const vec3 &p)
		{
			if (!l.isLine())
				return distance(l.a(), p);
			CAGE_THROW_CRITICAL(notImplemented, "geometry");
		}

		class intersectionDetector
		{
		public:
			const collisionObjectImpl *col;
			transform m;

			intersectionDetector(const collisionObjectImpl *collider, const transform &m) : col(collider), m(m)
			{}

			template<class T>
			real distance(const T &l, uint32 nodeIdx)
			{
				aabb b = col->boxes[nodeIdx];
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
						const triangle &t = col->tris[ti];
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
				aabb b = col->boxes[nodeIdx];
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
						const triangle &t = col->tris[ti];
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

			vec3 intersection(const line &l, uint32 nodeIdx)
			{
				aabb b = col->boxes[nodeIdx];
				if (!cage::intersects(l, b))
					return vec3::Nan();
				const auto &n = col->nodes[nodeIdx];
				if (n.left == cage::m)
				{ // node
					vec3 c1 = intersection(l, nodeIdx + 1);
					vec3 c2 = intersection(l, n.right);
					if (c1.valid())
					{
						if (c2.valid())
						{
							if (dist(l, c1) < dist(l, c2))
								return c1;
							return c2;
						}
						return c1;
					}
					return c2;
				}
				else
				{ // leaf
					real d = real::Infinity();
					vec3 r = vec3::Nan();
					for (uint32 ti = n.left; ti != n.right; ti++)
					{
						const triangle &t = col->tris[ti];
						vec3 p = cage::intersection(l, t);
						if (p.valid())
						{
							real d1 = dist(l, p);
							if (d1 < d)
							{
								r = p;
								d = d1;
							}
						}
					}
					return r;
				}
			}

			vec3 intersection(const line &l)
			{
				vec3 r3 = intersection(l * inverse(m), 0);
				vec4 r4 = vec4(r3, 1) * mat4(m);
				return vec3(r4) / r4[3];
			}
		};
	}

	uint32 collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt, collisionPair *outputBuffer, uint32 bufferSize)
	{
		CAGE_ASSERT_RUNTIME(!ao->needsRebuild());
		CAGE_ASSERT_RUNTIME(!bo->needsRebuild());
		CAGE_ASSERT_RUNTIME(bufferSize > 0);
		if (ao->trianglesCount() > bo->trianglesCount())
		{
			collisionDetector<false> d((const collisionObjectImpl*)ao, (const collisionObjectImpl*)bo, transform(), transform(inverse(at) * bt), outputBuffer, bufferSize);
			return d.process();
		}
		else
		{
			collisionDetector<true> d((const collisionObjectImpl*)ao, (const collisionObjectImpl*)bo, transform(inverse(bt) * at), transform(), outputBuffer, bufferSize);
			return d.process();
		}
	}

	uint32 collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact, collisionPair *outputBuffer, uint32 bufferSize)
	{
		CAGE_ASSERT_RUNTIME(at1.scale == at2.scale);
		CAGE_ASSERT_RUNTIME(bt1.scale == bt2.scale);

		fractionBefore = real::Nan();
		fractionContact = real::Nan();

		// find approximate time range of contact
		real time1 = timeOfContact(ao, bo, at1, bt1, at2, bt2);
		if (!time1.valid())
			return 0;
		CAGE_ASSERT_RUNTIME(time1 >= 0 && time1 <= 1, time1);
		real time2 = 1 - timeOfContact(ao, bo, at2, bt2, at1, bt1);
		CAGE_ASSERT_RUNTIME(time2.valid());
		CAGE_ASSERT_RUNTIME(time2 >= 0 && time2 <= 1, time2);
		CAGE_ASSERT_RUNTIME(time2 >= time1, time2, time1);
		
		// find first contact
		real minSize = min(minSizeObject(ao, at1.scale), minSizeObject(bo, bt1.scale)) * 0.5;
		real maxDist = max(distance(interpolate(at1.position, at2.position, time1), interpolate(at1.position, at2.position, time2)),
						   distance(interpolate(bt1.position, bt2.position, time1), interpolate(bt1.position, bt2.position, time2)));
		real maxDiff = (maxDist > minSize ? (minSize / maxDist) : 1) * (time2 - time1);
		CAGE_ASSERT_RUNTIME(maxDiff > 0 && maxDiff <= 1, maxDiff);
		maxDiff = min(maxDiff, (time2 - time1) * 0.2);
		while (time1 <= time2)
		{
			if (collisionDetection(ao, bo, interpolate(at1, at2, time1), interpolate(bt1, bt2, time1)))
			{
				time2 = time1;
				time1 = max(time1 - maxDiff, 0);
				break;
			}
			else
				time1 += maxDiff;
		}
		if (time1 > time2)
			return 0;

		// improve collision precision by binary search
		fractionBefore = time1;
		fractionContact = time2;
		for (uint32 i = 0; i < 6; i++)
		{
			real time = (time1 + time2) * 0.5;
			if (collisionDetection(ao, bo, interpolate(at1, at2, time), interpolate(bt1, bt2, time)))
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
		CAGE_ASSERT_RUNTIME(fractionBefore >= 0 && fractionBefore <= 1, fractionBefore);
		CAGE_ASSERT_RUNTIME(fractionContact >= 0 && fractionContact <= 1, fractionContact);
		CAGE_ASSERT_RUNTIME(fractionBefore <= fractionContact, fractionBefore, fractionContact);

		// fill the whole output buffer
		uint32 res = collisionDetection(ao, bo, interpolate(at1, at2, fractionContact), interpolate(bt1, bt2, fractionContact), outputBuffer, bufferSize);
		CAGE_ASSERT_RUNTIME(res > 0);
		CAGE_ASSERT_RUNTIME(fractionBefore == 0 || !collisionDetection(ao, bo, interpolate(at1, at2, fractionBefore), interpolate(bt1, bt2, fractionBefore)));
		return res;
	}

	bool collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt)
	{
		collisionPair buf;
		return collisionDetection(ao, bo, at, bt, &buf, 1) > 0;
	}

	bool collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2)
	{
		real fractionBefore, fractionContact;
		collisionPair buf;
		return collisionDetection(ao, bo, at1, bt1, at2, bt2, fractionBefore, fractionContact, &buf, 1) > 0;
	}

	bool collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, real &fractionBefore, real &fractionContact)
	{
		collisionPair buf;
		return collisionDetection(ao, bo, at1, bt1, at2, bt2, fractionBefore, fractionContact, &buf, 1) > 0;
	}

	uint32 collisionDetection(const collisionMesh *ao, const collisionMesh *bo, const transform &at1, const transform &bt1, const transform &at2, const transform &bt2, collisionPair *outputBuffer, uint32 bufferSize)
	{
		real fractionBefore, fractionContact;
		return collisionDetection(ao, bo, at1, bt1, at2, bt2, fractionBefore, fractionContact, outputBuffer, bufferSize);
	}



	real distance(const line &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.distance(shape);
	}

	real distance(const triangle &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.distance(shape);
	}

	real distance(const plane &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.distance(shape);
	}

	real distance(const sphere &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.distance(shape);
	}

	real distance(const aabb &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.distance(shape);
	}

	real distance(const collisionMesh *ao, const collisionMesh *bo, const transform &at, const transform &bt)
	{
		CAGE_THROW_CRITICAL(notImplemented, "collisionMesh-collisionMesh distance");
	}



	bool intersects(const line &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const triangle &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const plane &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const sphere &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.intersects(shape);
	}

	bool intersects(const aabb &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.intersects(shape);
	}



	vec3 intersection(const line &shape, const collisionMesh *collider, const transform &t)
	{
		intersectionDetector d((const collisionObjectImpl*)collider, t);
		return d.intersection(shape);
	}
}
