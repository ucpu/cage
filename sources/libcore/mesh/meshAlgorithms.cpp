#include "mesh.h"

#include <cage-core/meshAlgorithms.h>
#include <cage-core/geometry.h>
#include <cage-core/macros.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/image.h>
#include <cage-core/tasks.h>
#include <cage-core/flatSet.h>
#include <cage-core/collider.h>
#include <algorithm> // std::erase_if
#include <iterator> // back_insterter
#include <numeric> // std::iota
#include <array>

namespace cage
{
	namespace
	{
		template<class T>
		void turnLeft(T &a, T &b, T &c)
		{
			std::swap(a, b); // bac
			std::swap(b, c); // bca
		}

		template<class T> requires(std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3>)
		Vec2 barycoord(const T &a, const T &b, const T &c, const T &p)
		{
			const T v0 = b - a;
			const T v1 = c - a;
			const T v2 = p - a;
			const Real d00 = dot(v0, v0);
			const Real d01 = dot(v0, v1);
			const Real d11 = dot(v1, v1);
			const Real d20 = dot(v2, v0);
			const Real d21 = dot(v2, v1);
			const Real denom = d00 * d11 - d01 * d01;
			if (abs(denom) < 1e-7)
				return Vec2(); // the triangle is too small or degenerated, just return anything valid
			const Real invDenom = 1.0 / denom;
			const Real v = (d11 * d20 - d01 * d21) * invDenom;
			const Real w = (d00 * d21 - d01 * d20) * invDenom;
			const Real u = 1 - v - w;
			CAGE_ASSERT(u.valid() && v.valid());
			return Vec2(u, v);
		}

		Vec2i operator * (const Vec2i &a, Real b)
		{
			return Vec2i(sint32(a[0] * b.value), sint32(a[1] * b.value));
		}

		template<class T>
		void vectorEraseIf(std::vector<T> &v, const std::vector<bool> &toRemove)
		{
			CAGE_ASSERT(v.size() == toRemove.size());
			auto flagit = toRemove.begin();
			std::erase_if(v, [&](T&) {
				return *flagit++;
			});
			CAGE_ASSERT(flagit == toRemove.end());
		}

		void removeVertices(MeshImpl *impl, const std::vector<bool> &verticesToRemove)
		{
			CAGE_ASSERT(impl->verticesCount() == verticesToRemove.size());

			if (!impl->indices.empty())
			{
				std::vector<uint32> mapping; // mapping[original_index] = new_index
				mapping.reserve(verticesToRemove.size());
				{
					uint32 removed = 0;
					uint32 index = 0;
					for (bool b : verticesToRemove)
					{
						CAGE_ASSERT(removed <= index);
						mapping.push_back(index - removed);
						index++;
						removed += b;
					}
				}
				for (uint32 &i : impl->indices)
				{
					CAGE_ASSERT(i < verticesToRemove.size());
					CAGE_ASSERT(!verticesToRemove[i]);
					i = mapping[i];
				}
			}

#define GCHL_GENERATE(NAME) if (!impl->NAME.empty()) vectorEraseIf(impl->NAME, verticesToRemove);
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		}

		void removeUnusedVertices(MeshImpl *impl)
		{
			if (impl->indices.empty())
				return;
			std::vector<bool> verticesToRemove;
			verticesToRemove.resize(impl->positions.size(), true);
			for (uint32 i : impl->indices)
				verticesToRemove[i] = false;
			removeVertices(impl, verticesToRemove);
		}

		// mark entire triplet (triangles) or pair (lines) if any one of them is marked
		void markFacesWithInvalidVertices(MeshTypeEnum type, std::vector<bool> &marks)
		{
			switch (type)
			{
			case MeshTypeEnum::Points:
				break;
			case MeshTypeEnum::Lines:
			{
				const uint32 cnt = numeric_cast<uint32>(marks.size()) / 2;
				for (uint32 i = 0; i < cnt; i++)
				{
					if (marks[i * 2 + 0] || marks[i * 2 + 1])
						marks[i * 2 + 0] = marks[i * 2 + 1] = true;
				}
			} break;
			case MeshTypeEnum::Triangles:
			{
				const uint32 cnt = numeric_cast<uint32>(marks.size()) / 3;
				for (uint32 i = 0; i < cnt; i++)
				{
					if (marks[i * 3 + 0] || marks[i * 3 + 1] || marks[i * 3 + 2])
						marks[i * 3 + 0] = marks[i * 3 + 1] = marks[i * 3 + 2] = true;
				}
			} break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid mesh type");
			}
		}

		PointerRange<Triangle> planeCut(const Plane &plane, const Triangle &in, Triangle out[3])
		{
			const Vec3 a = intersection(plane, makeSegment(in[0], in[1]));
			const Vec3 b = intersection(plane, makeSegment(in[1], in[2]));
			const Vec3 c = intersection(plane, makeSegment(in[2], in[0]));
			const bool lns[3] = { valid(a), valid(b), valid(c) };
			const uint32 lnc = lns[0] + lns[1] + lns[2];
			if (lnc != 2)
			{
				out[0] = in;
				return { out, out + 1 };
			}

			Triangle r;
			std::array<Vec3, 3> mids;
			if (!lns[0])
			{
				r = in;
				mids = { a, b, c };
			}
			if (!lns[1])
			{
				r = Triangle(in[1], in[2], in[0]);
				mids = { b, c, a };
			}
			if (!lns[2])
			{
				r = Triangle(in[2], in[0], in[1]);
				mids = { c, a, b };
			}

			CAGE_ASSERT(!valid(mids[0]));
			CAGE_ASSERT(valid(mids[1]));
			CAGE_ASSERT(valid(mids[2]));
			out[0] = Triangle(r[0], r[1], mids[1]);
			out[1] = Triangle(r[0], mids[1], mids[2]);
			out[2] = Triangle(mids[1], r[2], mids[2]);
			return { out, out + 3 };
		}
	}

	void meshConvertToIndexed(Mesh *msh)
	{
		if (!msh->indices().empty() || msh->positions().empty())
			return;

		struct Comparator
		{
			bool operator() (const Vec3 &a, const Vec3 &b) const
			{
				return detail::memcmp(&a, &b, sizeof(a)) < 0;
			}
		};
		const FlatSet pos = makeFlatSet<Vec3, Comparator>(msh->positions());

		std::vector<uint32> inds;
		inds.reserve(msh->positions().size());
		for (const Vec3 &p : msh->positions())
		{
			const auto it = pos.find(p);
			CAGE_ASSERT(it != pos.end());
			inds.push_back(it - pos.begin());
		}

		// todo other attributes

		const MeshTypeEnum type = msh->type();
		msh->clear();
		msh->type(type);
		msh->positions(pos);
		msh->indices(inds);
	}

	void meshConvertToExpanded(Mesh *msh)
	{
		if (msh->indices().empty())
			return;

		std::vector<Vec3> ps;
		ps.reserve(msh->indicesCount());

		for (uint32 i : msh->indices())
			ps.push_back(msh->position(i));

		// todo other attributes

		const MeshTypeEnum type = msh->type();
		msh->clear();
		msh->type(type);
		msh->positions(ps);
	}

	void meshApplyTransform(Mesh *msh, const Transform &t)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		for (Vec3 &it : impl->positions)
			it = t * it;
		for (Vec3 &it : impl->normals)
			it = t.orientation * it;
		for (Vec3 &it : impl->tangents)
			it = t.orientation * it;
	}

	void meshApplyTransform(Mesh *msh, const Mat4 &t)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		for (Vec3 &it : impl->positions)
			it = Vec3(t * Vec4(it, 1));
		for (Vec3 &it : impl->normals)
			it = Vec3(t * Vec4(it, 0));
		for (Vec3 &it : impl->tangents)
			it = Vec3(t * Vec4(it, 0));
	}

	void meshApplyAnimation(Mesh *msh, PointerRange<const Mat4> skinTransformation)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		const uint32 cnt = msh->verticesCount();
		if (impl->boneIndices.size() != cnt || impl->boneWeights.size() != cnt)
			CAGE_THROW_ERROR(Exception, "applying skin animation requires bones");
		for (uint32 i = 0; i < cnt; i++)
		{
			Mat4 tr = Mat4::scale(0);
			for (uint32 j = 0; j < 4; j++)
			{
				const uint32 b = impl->boneIndices[i][j];
				const Real w = impl->boneWeights[i][j];
				tr += skinTransformation[b] * w;
			}
			CAGE_ASSERT(tr.valid());
			impl->positions[i] = Vec3(tr * Vec4(impl->positions[i], 1));
			const Mat3 tr3 = Mat3(tr);
			if (!impl->normals.empty())
				impl->normals[i] = tr3 * impl->normals[i];
			if (!impl->tangents.empty())
				impl->tangents[i] = tr3 * impl->tangents[i];
		}
		// erase bone attributes to prevent repeatedly applying animations
		impl->boneIndices.clear();
		impl->boneWeights.clear();
	}

	void meshFlipNormals(Mesh *msh)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		if (impl->type == MeshTypeEnum::Triangles)
		{
			// flip triangle winding
			meshConvertToIndexed(msh);
			const uint32 cnt = impl->indicesCount();
			for (uint32 i = 0; i < cnt; i += 3)
				std::swap(impl->indices[i + 1], impl->indices[i + 2]);
		}
		for (Vec3 &n : impl->normals)
			n *= -1; // todo should normals be reflected by the plane of the triangle instead?
		// todo tangents and other attributes
	}

	void meshDuplicateSides(Mesh *msh)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh duplicate sides requires triangles mesh");
		meshConvertToIndexed(msh);

		struct T
		{
			uint32 a, b, c;

			bool operator < (const T &other) const
			{
				return detail::memcmp(this, &other, sizeof(*this)) < 0;
			}

			T &order()
			{
				const uint32 ll = min(a, min(b, c));
				while (a != ll)
					turnLeft(a, b, c);
				return *this;
			}
		};

		const uint32 origCnt = msh->indicesCount();
		const auto orig = msh->indices();
		std::vector<T> vec;
		vec.reserve(msh->facesCount() * 2);
		for (uint32 i = 0; i < origCnt; i += 3)
		{
			vec.push_back(T{ orig[i + 0], orig[i + 1], orig[i + 2] }.order());
			vec.push_back(T{ orig[i + 0], orig[i + 2], orig[i + 1] }.order()); // flip triangle winding
		}
		const FlatSet ts = makeFlatSet<T>(std::move(vec));

		MeshImpl *impl = (MeshImpl *)msh;
		impl->indices.clear();
		impl->indices.reserve(ts.size() * 3);
		for (const T &t : ts)
		{
			impl->indices.push_back(t.a);
			impl->indices.push_back(t.b);
			impl->indices.push_back(t.c);
		}

		// todo normals and other attributes
	}

	namespace
	{
		struct UnionFind
		{
			void init(uint32 cnt)
			{
				data.clear();
				data.resize(cnt);
				std::iota(data.begin(), data.end(), (uint32)0);
			}

			uint32 representative(uint32 x)
			{
				uint32 &y = data[x];
				if (y == x)
					return x;
				return y = representative(y);
			}

			void merge(uint32 a, uint32 b)
			{
				const uint32 x = representative(a);
				const uint32 y = representative(b);
				CAGE_ASSERT(x <= a);
				CAGE_ASSERT(y <= b);
				if (data[x] > y)
					data[x] = y;
				else
					data[y] = x;
			}

			std::vector<std::vector<uint32>> groups(bool all = false)
			{
				const uint32 cnt = data.size();
				std::vector<std::vector<uint32>> res;
				res.resize(cnt);
				for (uint32 i = 0; i < cnt; i++)
					res[representative(i)].push_back(i);
				if (all)
					std::erase_if(res, [](const auto &v) { return v.empty(); });
				else
					std::erase_if(res, [](const auto &v) { return v.size() < 2; });
				return res;
			}

		private:
			std::vector<uint32> data;
		};

#ifdef CAGE_DEBUG
		struct UnionFindTester
		{
			UnionFindTester()
			{
				UnionFind r;
				r.init(7); // 0 | 1 | 2 | 3 | 4 | 5 | 6
				r.merge(2, 4); // 0 | 1 | 2, 4 | 3 | 5 | 6
				r.merge(3, 1); // 0 | 1, 3 | 2, 4 | 5 | 6
				r.merge(4, 2); // 0 | 1, 3 | 2, 4 | 5 | 6
				r.merge(0, 5); // 0, 5 | 1, 3 | 2, 4 | 6
				r.merge(5, 2); // 0, 2, 4, 5 | 1, 3 | 6
				const auto g = r.groups();
				CAGE_ASSERT(g.size() == 2); // the 6 is not changed so not reported
				CAGE_ASSERT(g[0].size() == 4);
				CAGE_ASSERT(g[0] == std::vector<uint32>({ 0, 2, 4, 5 }));
				CAGE_ASSERT(g[1].size() == 2);
				CAGE_ASSERT(g[1] == std::vector<uint32>({ 1, 3 }));
			}
		} unionFindTester;
#endif // CAGE_DEBUG
	}

	void meshMergeCloseVertices(Mesh *msh, const MeshMergeCloseVerticesConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (!config.moveVerticesOnly)
			meshConvertToIndexed(msh);

		PointerRange<const Vec3> ps = msh->positions();
		const uint32 vc = numeric_cast<uint32>(msh->positions().size());
		UnionFind remap;
		remap.init(vc);

		// find which vertices can be remapped to other vertices
		{
			Holder<SpatialStructure> ss = newSpatialStructure({});
			for (uint32 i = 0; i < vc; i++)
				ss->update(i, ps[i]);
			ss->rebuild();
			Holder<SpatialQuery> q = newSpatialQuery(ss.share());
			for (uint32 i = 0; i < vc; i++)
			{
				q->intersection(Aabb(ps[i] - config.distanceThreshold, ps[i] + config.distanceThreshold));
				for (uint32 j : q->result())
					remap.merge(i, j);
			}
		}

		// apply changes
		MeshImpl *impl = (MeshImpl *)msh;
		if (config.moveVerticesOnly)
		{
			// recenter vertices
			const auto groups = remap.groups();
			for (const auto &it : groups)
			{
				Vec3 p;
				for (uint32 j : it)
					p += ps[j];
				p /= it.size();
				for (uint32 j : it)
					impl->positions[j] = p;
			}
		}
		else
		{
			// reassign indices
			for (uint32 &i : impl->indices)
				i = remap.representative(i);
		}

		// remove faces that has collapsed
		meshRemoveInvalid(msh);
	}

	void meshMergePlanar(Mesh *msh, const MeshMergePlanarConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh merge planar requires triangles mesh");
		meshConvertToIndexed(msh);

		const uint32 vc = msh->verticesCount();
		const uint32 tc = msh->facesCount();
		const auto poss = msh->positions();
		const auto inds = msh->indices();

		// triangle normals
		std::vector<Vec3> triNorms;
		triNorms.reserve(tc);

		// triangles sharing particular vertices
		std::vector<FlatSet<uint32>> inverseMapping; // vertex index -> set of triangles
		inverseMapping.resize(vc);

		for (uint32 ti = 0; ti < tc; ti++)
		{
			inverseMapping[inds[ti * 3 + 0]].insert(ti);
			inverseMapping[inds[ti * 3 + 1]].insert(ti);
			inverseMapping[inds[ti * 3 + 2]].insert(ti);
			triNorms.push_back(Triangle(poss[inds[ti * 3 + 0]], poss[inds[ti * 3 + 1]], poss[inds[ti * 3 + 2]]).normal());
		}

		// are 3 points collinear (must be in order)
		const auto &isColinear = [&](uint32 a, uint32 b, uint32 c) -> bool {
			const Vec3 x = normalize(poss[b] - poss[a]);
			const Vec3 y = normalize(poss[c] - poss[b]);
			return dot(x, y) > 0.999;
		};

		// triangles that has already changed
		std::vector<bool> banned;
		banned.resize(tc, false);

		// new indices (preserve original unmodified indices inside the mesh)
		std::vector<uint32> indsCopy = std::vector<uint32>(inds.begin(), inds.end());

		// verify if merging vertex v1 to v2 is valid for all triangles tris and do it
		// it is valid to degenerate triangles but not to flip orientation
		const auto &checkAndMergeVertex = [&](PointerRange<const uint32> tris, uint32 v1, uint32 v2) -> bool {
			CAGE_ASSERT(v1 != v2);
			const auto &loop = [&](bool perform) -> bool {
				for (uint32 ti : tris)
				{
					uint32 is[3] = { inds[ti * 3 + 0], inds[ti * 3 + 1], inds[ti * 3 + 2] };
					for (uint32 &i : is)
						if (i == v1)
							i = v2;
					CAGE_ASSERT((is[0] == v2) + (is[1] == v2) + (is[2] == v2) >= 1);
					const Triangle t = Triangle(poss[is[0]], poss[is[1]], poss[is[2]]);
					if (perform)
					{
						CAGE_ASSERT(!banned[ti]);
						banned[ti] = true;
						indsCopy[ti * 3 + 0] = is[0];
						indsCopy[ti * 3 + 1] = is[1];
						indsCopy[ti * 3 + 2] = is[2];
					}
					else
					{
						if (dot(t.normal(), triNorms[ti]) < 0 && !t.degenerated())
							return false;
					}
				}
				return true;
			};
			if (!loop(false))
				return false;
			loop(true);
			return true;
		};

		// find vertices that can be merged elsewhere
		std::vector<uint32> group;
		std::vector<uint32> edgeVerts;
		std::unordered_map<uint32, uint32> trisOnVertsCount;
		for (uint32 vi = 0; vi < vc; vi++)
		{
			for (uint32 t1 : inverseMapping[vi])
			{
				// find triangles that are coplanar with t1
				group.clear();
				for (uint32 t2 : inverseMapping[vi])
				{
					if (t2 <= t1)
						continue;
					if (dot(triNorms[t1], triNorms[t2]) > 0.999)
						group.push_back(t2);
				}
				if (group.empty())
					continue;
				group.push_back(t1);

				// find if any of the triangles in the group is banned
				if ([&] {
					for (uint32 i : group)
						if (banned[i])
							return true;
					return false;
				}())
					continue; // some triangle is banned

				// check if the vi vertex is fully surrounded or on an edge
				trisOnVertsCount.clear();
				for (uint32 ti : group)
				{
					trisOnVertsCount[inds[ti * 3 + 0]]++;
					trisOnVertsCount[inds[ti * 3 + 1]]++;
					trisOnVertsCount[inds[ti * 3 + 2]]++;
				}
				CAGE_ASSERT(trisOnVertsCount.at(vi) == group.size());
				uint32 ones = 0;
				uint32 twos = 0;
				edgeVerts.clear();
				for (const auto &it : trisOnVertsCount)
				{
					switch (it.second)
					{
					case 1:
						ones++;
						edgeVerts.push_back(it.first);
						break;
					case 2:
						twos++;
						break;
					}
				}
				if (ones != 2 && twos != group.size())
					continue; // the vertex cannot be merged

				// if the vertex is on edge
				if (edgeVerts.size() == 2)
				{
					if (!isColinear(edgeVerts[0], vi, edgeVerts[1]))
						continue; // not straight
					if (checkAndMergeVertex(group, vi, edgeVerts[0]))
						continue;
					checkAndMergeVertex(group, vi, edgeVerts[1]);
					continue;
				}

				// the vertex is fully surrounded
				for (const auto &it : trisOnVertsCount)
				{
					if (it.second == 2)
					{
						if (checkAndMergeVertex(group, vi, it.first))
							break;
					}
				}
			}
		}

		// finish the changes
		msh->indices(indsCopy);
		meshRemoveInvalid(msh);

		// repeat until it cannot improve
		if (msh->facesCount() < tc)
			meshMergePlanar(msh, config);
	}

	Holder<Mesh> meshCut(Mesh *msh, const Plane &pln)
	{
		Holder<Mesh> b = msh->copy();
		meshClip(msh, pln);
		meshClip(+b, Plane(pln.origin(), -pln.normal));
		return b;
	}

	namespace
	{
		Holder<PointerRange<Holder<Mesh>>> splitComponentsTriangles(const MeshImpl *src)
		{
			UnionFind comps;
			comps.init(src->positions.size());
			const uint32 trisCnt = numeric_cast<uint32>(src->indices.size() / 3);
			for (uint32 t = 0; t < trisCnt; t++)
			{
				comps.merge(src->indices[t * 3 + 0], src->indices[t * 3 + 1]);
				comps.merge(src->indices[t * 3 + 1], src->indices[t * 3 + 2]);
			}

			const uint32 indsCount = numeric_cast<uint32>(src->indices.size());
			std::vector<uint32> inds;
			inds.reserve(indsCount);
			PointerRangeHolder<Holder<Mesh>> result;
			const auto groups = comps.groups(true);
			for (const auto &grp : groups)
			{
				const uint32 gid = grp[0];
				Holder<Mesh> p = src->copy();
				for (uint32 t = 0; t < trisCnt; t++)
				{
					const uint32 tid = comps.representative(src->indices[t * 3 + 0]);
					if (gid != tid)
						continue;
					inds.push_back(src->indices[t * 3 + 0]);
					inds.push_back(src->indices[t * 3 + 1]);
					inds.push_back(src->indices[t * 3 + 2]);
				}
				p->indices(inds);
				removeUnusedVertices((MeshImpl *)+p);
				result.push_back(std::move(p));
				inds.clear();
			}
			return result;
		}
	}

	Holder<PointerRange<Holder<Mesh>>> meshSeparateDisconnected(const Mesh *msh)
	{
		msh->verticesCount(); // validate vertices
		if (msh->facesCount() == 0 || msh->type() == MeshTypeEnum::Points)
			return {};
		const MeshImpl *impl = (const MeshImpl *)msh;
		Holder<Mesh> srcCopy;
		if (msh->indicesCount() == 0)
		{
			srcCopy = msh->copy();
			meshConvertToIndexed(+srcCopy);
			impl = (const MeshImpl *)+srcCopy;
		}
		CAGE_ASSERT(!impl->indices.empty());

		switch (impl->type)
		{
		case MeshTypeEnum::Lines:
			CAGE_THROW_CRITICAL(NotImplemented, "separateDisconnected");
		case MeshTypeEnum::Triangles:
			return splitComponentsTriangles(impl);
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid mesh type");
		}
	}

	namespace
	{
		Plane longTriangleCuttingPlane(const Triangle &t)
		{
			const Real lengths[3] = { distanceSquared(t[0], t[1]), distanceSquared(t[1], t[2]), distanceSquared(t[2], t[0]) };
			const Real ll = max(max(lengths[0], lengths[1]), lengths[2]);
			Line l;
			for (uint32 i = 0; i < 3; i++)
				if (lengths[i] == ll)
					l = makeSegment(t[i], t[(i + 1) % 3]);
			const Vec3 n = l.direction;
			CAGE_ASSERT(valid(n));
			return Plane((l.a() + l.b()) * 0.5, n);
		}
	}

	void meshSplitLong(Mesh *msh, const MeshSplitLongConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh split long requires triangles mesh");
		if (config.ratio < 1e-5 || config.length < 0)
			CAGE_THROW_ERROR(Exception, "mesh split long requires valid config");
		meshConvertToIndexed(msh);

		std::vector<Triangle> a;
		{
			a.reserve(msh->facesCount() * 2);
			const uint32 incnt = msh->indicesCount();
			const auto ins = msh->indices();
			const auto pos = msh->positions();
			for (uint32 i = 0; i < incnt; i += 3)
				a.push_back(Triangle(pos[ins[i + 0]], pos[ins[i + 1]], pos[ins[i + 2]]));
		}
		std::vector<Triangle> b, f;
		b.reserve(a.size());
		f.reserve(a.size());

		Triangle tmp[3];
		while (!a.empty())
		{
			for (const Triangle &t : a)
			{
				Real lengths[3] = { distance(t[0], t[1]), distance(t[1], t[2]), distance(t[2], t[0]) };
				std::sort(std::begin(lengths), std::end(lengths));
				if (lengths[0] / lengths[2] > config.ratio || lengths[2] < config.length)
				{
					f.push_back(t);
					continue;
				}
				const auto r = planeCut(longTriangleCuttingPlane(t), t, tmp);
				if (r.size() == 1)
					f.push_back(r[0]);
				else if (r.size() > 1)
				{
					for (const Triangle &k : r)
						if (!k.degenerated())
							b.push_back(k);
				}
			}
			std::swap(a, b);
			b.clear();
		}

		msh->clear();
		for (const Triangle &t : f)
			msh->addTriangle(t);

		meshRemoveInvalid(msh);
	}

	void meshSplitIntersecting(Mesh *msh, const MeshSplitIntersectingConfig &config)
	{
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh split intersecting requires triangles mesh");

		struct Processor
		{
			Mesh *msh = nullptr;
			const MeshSplitIntersectingConfig &config;
			Holder<Collider> collider = newCollider();
			Holder<SpatialStructure> spatialStructure = newSpatialStructure({});
			std::vector<std::vector<Triangle>> tasks;

			Processor(Mesh *msh, const MeshSplitIntersectingConfig &config) : msh(msh), config(config)
			{}

			void operator()(uint32 triIdx)
			{
				std::vector<Triangle> &tris = tasks[triIdx];
				CAGE_ASSERT(tris.size() == 1);
				const Triangle origTri = tris[0];
				Holder<SpatialQuery> q = newSpatialQuery(spatialStructure.share());
				q->intersection(origTri);
				auto cutterIds = q->result();
				if (cutterIds.size() >= config.maxCuttersForTriangle)
					return;
				const PointerRange<const Triangle> cctris = collider->triangles();
				std::sort(cutterIds.begin(), cutterIds.end(), [cctris](uint32 a, uint32 b) {
					return cctris[a].area() > cctris[b].area(); // cut with largest triangles first
				});
				std::vector<Triangle> b;
				b.reserve(10);
				Triangle tmp[3];
				for (const uint32 qi : cutterIds)
				{
					const Triangle &cutterTri = cctris[qi];
					CAGE_ASSERT(intersects(cutterTri, origTri));
					const Plane cutterPl = Plane(cutterTri);
					for (const Triangle &t : tris)
					{
						for (const Triangle &k : planeCut(cutterPl, t, tmp))
							if (!k.degenerated())
								b.push_back(k);
					}
					std::swap(tris, b);
					b.clear();
				}
			}

			void run()
			{
				meshConvertToIndexed(msh);

				{ // prepare triangles used for cutting
					collider->importMesh(msh);
					collider->optimize();
					uint32 idx = 0;
					for (const Triangle &t : collider->triangles())
						spatialStructure->update(idx++, t);
					spatialStructure->rebuild();
				}

				{ // prepare triangles to be cut
					tasks.reserve(msh->facesCount());
					const uint32 incnt = msh->indicesCount();
					const auto ins = msh->indices();
					const auto pos = msh->positions();
					for (uint32 i = 0; i < incnt; i += 3)
					{
						const Triangle t = Triangle(pos[ins[i + 0]], pos[ins[i + 1]], pos[ins[i + 2]]);
						std::vector<Triangle> tsk;
						tsk.reserve(10);
						tsk.push_back(t);
						tasks.push_back(std::move(tsk));
					}
				}

				if (config.parallelize)
					tasksRunBlocking<Processor>("meshSplitIntersecting", *this, tasks.size());
				else
				{
					const uint32 sz = tasks.size();
					for (uint32 i = 0; i < sz; i++)
						operator()(i);
				}

				msh->clear();
				for (const auto &tsk : tasks)
					for (const Triangle &t : tsk)
						msh->addTriangle(t);

				meshRemoveInvalid(msh);
			}
		};

		Processor proc(msh, config);
		proc.run();
	}

	namespace
	{
		uint32 clipAddPoint(MeshImpl *impl, const uint32 ai, const uint32 bi, const uint32 axis, const Real value)
		{
			const Vec3 a = impl->positions[ai];
			const Vec3 b = impl->positions[bi];
			if (abs(b[axis] - a[axis]) < 1e-5)
				return ai; // the edge is very short or degenerated
			const Real pu = (value - a[axis]) / (b[axis] - a[axis]);
			CAGE_ASSERT(pu >= 0 && pu <= 1);
			if (pu < 1e-5)
				return ai; // the cut is very close to the beginning of the line
			if (pu > 1 - 1e-5)
				return bi; // the cut is very close to the end of the line
			impl->positions.push_back(interpolate(a, b, pu));
			if (!impl->normals.empty())
				impl->normals.push_back(normalize(interpolate(impl->normals[ai], impl->normals[bi], pu)));
			if (!impl->uvs.empty())
				impl->uvs.push_back(interpolate(impl->uvs[ai], impl->uvs[bi], pu));
			if (!impl->uvs3.empty())
				impl->uvs3.push_back(interpolate(impl->uvs3[ai], impl->uvs3[bi], pu));
			return numeric_cast<uint32>(impl->positions.size()) - 1;
		}

		void clipTriangles(MeshImpl *impl, const std::vector<uint32> &in, std::vector<uint32> &out, const uint32 axis, const Real value, const bool side)
		{
			CAGE_ASSERT((in.size() % 3) == 0);
			CAGE_ASSERT(out.size() == 0);
			const uint32 tris = numeric_cast<uint32>(in.size() / 3);
			for (uint32 tri = 0; tri < tris; tri++)
			{
				uint32 ids[3] = { in[tri * 3 + 0], in[tri * 3 + 1], in[tri * 3 + 2] };
				Vec3 a = impl->positions[ids[0]];
				Vec3 b = impl->positions[ids[1]];
				Vec3 c = impl->positions[ids[2]];
				bool as = a[axis] > value;
				bool bs = b[axis] > value;
				bool cs = c[axis] > value;
				const uint32 m = as + bs + cs;
				if ((m == 0 && side) || (m == 3 && !side))
				{
					// all passes
					out.push_back(ids[0]);
					out.push_back(ids[1]);
					out.push_back(ids[2]);
					continue;
				}
				if ((m == 0 && !side) || (m == 3 && side))
				{
					// all rejected
					continue;
				}
				CAGE_ASSERT(m == 1 || m == 2);
				while (as || !bs)
				{
					turnLeft(ids[0], ids[1], ids[2]);
					turnLeft(a, b, c);
					turnLeft(as, bs, cs);
				}
				CAGE_ASSERT(!as && bs);
				const uint32 ab = clipAddPoint(impl, ids[0], ids[1], axis, value);
				if (m == 1)
				{
					CAGE_ASSERT(!as);
					CAGE_ASSERT(bs);
					CAGE_ASSERT(!cs);
					/*
					*         |
					* a +-------+ b
					*    \    |/
					*     \   /
					*      \ /|
					*     c + |
					*/
					const uint32 bc = clipAddPoint(impl, ids[1], ids[2], axis, value);
					if (side)
					{
						out.push_back(ids[0]);
						out.push_back(ab);
						out.push_back(bc);

						out.push_back(ids[0]);
						out.push_back(bc);
						out.push_back(ids[2]);
					}
					else
					{
						out.push_back(ab);
						out.push_back(ids[1]);
						out.push_back(bc);
					}
				}
				else
				{
					CAGE_ASSERT(m == 2);
					CAGE_ASSERT(!as);
					CAGE_ASSERT(bs);
					CAGE_ASSERT(cs);
					/*
					*     |
					* a +-------+ b
					*    \|    /
					*     \   /
					*     |\ /
					*     | + c
					*/
					const uint32 ac = clipAddPoint(impl, ids[0], ids[2], axis, value);
					if (side)
					{
						out.push_back(ids[0]);
						out.push_back(ab);
						out.push_back(ac);
					}
					else
					{
						out.push_back(ab);
						out.push_back(ids[1]);
						out.push_back(ac);

						out.push_back(ac);
						out.push_back(ids[1]);
						out.push_back(ids[2]);
					}
				}
			}
			CAGE_ASSERT((out.size() % 3) == 0);
		}
	}

	void meshClip(Mesh *msh, const Aabb &clipBox)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh clip requires triangles mesh");

		meshConvertToIndexed(msh);
		MeshImpl *impl = (MeshImpl *)msh;
		const Vec3 clipBoxArr[2] = { clipBox.a, clipBox.b };
		std::vector<uint32> sourceIndices;
		sourceIndices.reserve(impl->indices.size());
		sourceIndices.swap(impl->indices);
		std::vector<uint32> tmp, tmp2;
		const uint32 tris = numeric_cast<uint32>(sourceIndices.size() / 3);
		for (uint32 tri = 0; tri < tris; tri++)
		{
			const uint32 *ids = sourceIndices.data() + tri * 3;
			{
				const Vec3 &a = impl->positions[ids[0]];
				const Vec3 &b = impl->positions[ids[1]];
				const Vec3 &c = impl->positions[ids[2]];
				if (intersects(a, clipBox) && intersects(b, clipBox) && intersects(c, clipBox))
				{
					// triangle fully inside the box
					impl->indices.push_back(ids[0]);
					impl->indices.push_back(ids[1]);
					impl->indices.push_back(ids[2]);
					continue;
				}
				if (!intersects(Triangle(a, b, c), clipBox))
					continue; // triangle fully outside
			}
			CAGE_ASSERT(tmp.empty());
			tmp.push_back(ids[0]);
			tmp.push_back(ids[1]);
			tmp.push_back(ids[2]);
			for (uint32 axis = 0; axis < 3; axis++)
			{
				for (uint32 side = 0; side < 2; side++)
				{
					clipTriangles(impl, tmp, tmp2, axis, clipBoxArr[side][axis], side);
					tmp.swap(tmp2);
					tmp2.clear();
				}
			}
			CAGE_ASSERT((tmp.size() % 3) == 0);
			for (uint32 i : tmp)
				impl->indices.push_back(i);
			tmp.clear();
		}
		if (impl->indices.empty())
			msh->clear();
		else
			removeUnusedVertices(impl);
		meshRemoveInvalid(impl);
	}

	void meshClip(Mesh *msh, const Plane &pln)
	{
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh clip requires triangles mesh");
		meshConvertToIndexed(msh);

		std::vector<Triangle> b;
		b.reserve(msh->facesCount() * 2);

		const uint32 incnt = msh->indicesCount();
		const auto ins = msh->indices();
		const auto pos = msh->positions();
		Triangle tmp[3];
		for (uint32 i = 0; i < incnt; i += 3)
		{
			const Triangle t = Triangle(pos[ins[i + 0]], pos[ins[i + 1]], pos[ins[i + 2]]);
			for (const Triangle &k : planeCut(pln, t, tmp))
				if (dot(k.center() - pln.origin(), pln.normal) > 0)
					b.push_back(k);
		}

		msh->clear();
		for (const Triangle &t : b)
			msh->addTriangle(t);

		meshRemoveInvalid(msh);
	}

	namespace
	{
		void discardInvalidVertices(MeshImpl *impl)
		{
			std::vector<bool> verticesToRemove;
			verticesToRemove.resize(impl->positions.size(), false);
			{
				auto r = verticesToRemove.begin();
				for (const Vec3 &v : impl->positions)
				{
					*r = *r || !valid(v);
					r++;
				}
				CAGE_ASSERT(r == verticesToRemove.end());
			}
			{
				auto r = verticesToRemove.begin();
				for (const Vec2 &v : impl->uvs)
				{
					*r = *r || !valid(v);
					r++;
				}
			}
			{
				auto r = verticesToRemove.begin();
				for (const Vec3 &v : impl->uvs3)
				{
					*r = *r || !valid(v);
					r++;
				}
			}
			{
				auto r = verticesToRemove.begin();
				for (const Vec3 &v : impl->normals)
				{
					*r = *r || !valid(v) || abs(lengthSquared(v) - 1) > 1e-3;
					r++;
				}
			}
			if (impl->indices.empty())
			{
				markFacesWithInvalidVertices(impl->type, verticesToRemove);
			}
			else
			{
				std::vector<bool> invalidIndices;
				invalidIndices.reserve(impl->indices.size());
				for (uint32 i : impl->indices)
					invalidIndices.push_back(verticesToRemove[i]);
				markFacesWithInvalidVertices(impl->type, invalidIndices);
				vectorEraseIf(impl->indices, invalidIndices);
			}
			removeVertices(impl, verticesToRemove);
		}

		void discardInvalidLines(MeshImpl *impl)
		{
			// todo
		}

		void discardInvalidTriangles(MeshImpl *impl)
		{
			const uint32 tris = impl->facesCount();
			PointerRange<const Vec3> ps = impl->positions;
			if (impl->indices.empty())
			{
				std::vector<bool> verticesToRemove;
				verticesToRemove.reserve(tris * 3);
				for (uint32 i = 0; i < tris; i++)
				{
					const Triangle t(ps[i * 3 + 0], ps[i * 3 + 1], ps[i * 3 + 2]);
					const bool d = t.degenerated();
					verticesToRemove.push_back(d);
					verticesToRemove.push_back(d);
					verticesToRemove.push_back(d);
				}
				removeVertices(impl, verticesToRemove);
				CAGE_ASSERT(impl->positions.size() % 3 == 0);
			}
			else
			{
				PointerRange<const uint32> is = impl->indices;
				std::vector<bool> indicesToRemove;
				indicesToRemove.reserve(tris * 3);
				for (uint32 i = 0; i < tris; i++)
				{
					const Triangle t(ps[is[i * 3 + 0]], ps[is[i * 3 + 1]], ps[is[i * 3 + 2]]);
					const bool d = t.degenerated();
					indicesToRemove.push_back(d);
					indicesToRemove.push_back(d);
					indicesToRemove.push_back(d);
				}
				vectorEraseIf(impl->indices, indicesToRemove);
				if (impl->indices.empty())
					impl->clear();
				else
					removeUnusedVertices(impl);
				CAGE_ASSERT(impl->indices.size() % 3 == 0);
			}
		}
	}

	void meshRemoveInvalid(Mesh *msh)
	{
		msh->verticesCount(); // validate vertices
		MeshImpl *impl = (MeshImpl *)msh;
		discardInvalidVertices(impl);
		switch (impl->type)
		{
		case MeshTypeEnum::Points:
			break;
		case MeshTypeEnum::Lines:
			discardInvalidLines(impl);
			break;
		case MeshTypeEnum::Triangles:
			discardInvalidTriangles(impl);
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid mesh type");
		}
		msh->verticesCount(); // validate vertices
	}

	void meshRemoveDisconnected(Mesh *msh)
	{
		if (msh->facesCount() == 0)
			return;
		// todo optimized code without separateDisconnected
		auto vec = meshSeparateDisconnected(msh);
		uint32 largestIndex = 0;
		uint32 largestFaces = vec[0]->facesCount();
		for (uint32 i = 1; i < vec.size(); i++)
		{
			uint32 f = vec[i]->facesCount();
			if (f > largestFaces)
			{
				largestFaces = f;
				largestIndex = i;
			}
		}
		MeshImpl *impl = (MeshImpl *)msh;
		MeshImpl *src = (MeshImpl *)vec[largestIndex].get();
		impl->swap(*src);
	}

	void meshRemoveSmall(Mesh *msh, const MeshRemoveSmallConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (config.threshold <= 0)
			CAGE_THROW_ERROR(Exception, "mesh remove small requires positive threshold");
		meshConvertToIndexed(msh);

		const uint32 incnt = msh->indicesCount();
		const auto orig = msh->indices();
		const auto poss = msh->positions();
		std::vector<uint32> inds;
		inds.reserve(incnt);

		switch (msh->type())
		{
		case MeshTypeEnum::Triangles:
		{
			for (uint32 i = 0; i < incnt; i += 3)
			{
				const Vec3 a = poss[orig[i + 0]];
				const Vec3 b = poss[orig[i + 1]];
				const Vec3 c = poss[orig[i + 2]];
				if (Triangle(a, b, c).area() >= config.threshold)
				{
					inds.push_back(orig[i + 0]);
					inds.push_back(orig[i + 1]);
					inds.push_back(orig[i + 2]);
				}
			}
		} break;
		case MeshTypeEnum::Lines:
		{
			for (uint32 i = 0; i < incnt; i += 2)
			{
				const Vec3 a = poss[orig[i + 0]];
				const Vec3 b = poss[orig[i + 1]];
				if (distanceSquared(a, b) >= sqr(config.threshold))
				{
					inds.push_back(orig[i + 0]);
					inds.push_back(orig[i + 1]);
				}
			}
		} break;
		default:
			CAGE_THROW_ERROR(Exception, "mesh remove small requires triangles or lines");
		}

		msh->indices(inds);
		meshRemoveInvalid(+msh);
	}

	void meshRemoveOccluded(Mesh *msh, const MeshRemoveOccludedConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh removing invisible requires triangles mesh");
		if (config.maxRaysPerTriangle < config.minRaysPerTriangle || config.raysPerUnitArea <= 1e-5)
			CAGE_THROW_ERROR(Exception, "mesh removing invisible requires valid configuration");

		struct Processor
		{
			Mesh *msh = nullptr;
			const MeshRemoveOccludedConfig &config;
			Holder<Collider> collider = newCollider();
			std::vector<uint8> visible;
			Real grazingDot;

			Processor(Mesh *msh, const MeshRemoveOccludedConfig &config) : msh(msh), config(config)
			{}

			void operator()(uint32 triIdx)
			{
				const auto inds = msh->indices();
				const auto poss = msh->positions();
				const Triangle t = Triangle(poss[inds[triIdx * 3 + 0]], poss[inds[triIdx * 3 + 1]], poss[inds[triIdx * 3 + 2]]);
				const Vec3 n = t.normal();
				const uint32 raysCount = clamp(numeric_cast<uint32>(t.area() * config.raysPerUnitArea), config.minRaysPerTriangle, config.maxRaysPerTriangle);
				const Vec3 u = t[1] - t[0];
				const Vec3 v = t[2] - t[0];
				for (uint32 i = 0; i < raysCount; i++)
				{
					const Vec2 fs = randomChance2() * 0.5;
					const Vec3 o = t[0] + fs[0] * u + fs[1] * v + n * 1e-6;
					Vec3 d = randomDirection3();
					while (abs(dot(d, n)) < grazingDot)
						d = randomDirection3();
					if (!config.doubleSided && dot(d, n) < 0)
						d *= -1;
					const Line ray = makeRay(o, o + d);
					if (!intersects(ray, +collider, Transform()))
					{
						visible[triIdx] = 1;
						break;
					}
				}
			}

			void run()
			{
				grazingDot = cage::cos(Degs(90) - config.grazingAngle);

				meshConvertToIndexed(msh);
				visible.resize(msh->facesCount(), 0);
				collider->importMesh(msh);
				collider->rebuild();

				if (config.parallelize)
					tasksRunBlocking<Processor>("meshRemoveOccluded", *this, msh->facesCount());
				else
				{
					const uint32 cnt = msh->facesCount();
					for (uint32 i = 0; i < cnt; i++)
						operator()(i);
				}

				{
					const auto orig = msh->indices();
					std::vector<uint32> inds;
					inds.reserve(orig.size());
					const uint32 cnt = orig.size() / 3;
					for (uint32 i = 0; i < cnt; i++)
					{
						if (visible[i])
						{
							inds.push_back(orig[i * 3 + 0]);
							inds.push_back(orig[i * 3 + 1]);
							inds.push_back(orig[i * 3 + 2]);
						}
					}
					msh->indices(inds);
				}

				meshRemoveInvalid(+msh);
			}
		};

		Processor proc(msh, config);
		proc.run();
	}

	namespace
	{
		Real meshSurfaceArea(const Mesh *mesh)
		{
			const auto inds = mesh->indices();
			const auto poss = mesh->positions();
			const uint32 cnt = numeric_cast<uint32>(inds.size() / 3);
			Real result = 0;
			for (uint32 ti = 0; ti < cnt; ti++)
			{
				const Triangle t = Triangle(poss[inds[ti * 3 + 0]], poss[inds[ti * 3 + 1]], poss[inds[ti * 3 + 2]]);
				result += t.area();
			}
			return result;
		}

		uint32 boxLongestAxis(const Aabb &box)
		{
			const Vec3 mySizes = box.size();
			const Vec3 a = abs(dominantAxis(mySizes));
			if (a[0] == 1)
				return 0;
			if (a[1] == 1)
				return 1;
			return 2;
		}

		Aabb clippingBox(const Aabb &box, uint32 axis, Real pos, bool second = false)
		{
			const Vec3 c = box.center();
			const Vec3 hs = box.size() * 0.6; // slightly larger box to avoid clipping due to floating point imprecisions
			Aabb r = Aabb(c - hs, c + hs);
			if (second)
				r.a[axis] = pos;
			else
				r.b[axis] = pos;
			return r;
		}

		std::vector<Holder<Mesh>> meshChunkingImpl(Holder<Mesh> mesh, const MeshChunkingConfig &config)
		{
			const Real myArea = meshSurfaceArea(+mesh);
			if (myArea > config.maxSurfaceArea)
			{
				const Aabb myBox = mesh->boundingBox();
				const uint32 a = boxLongestAxis(myBox);
				Real bestSplitPosition = 0.5;
				Real bestSplitScore = Real::Infinity();
				for (Real position : { 0.3, 0.4, 0.45, 0.5, 0.55, 0.6, 0.7 })
				{
					Holder<Mesh> p = mesh->copy();
					meshClip(+p, clippingBox(myBox, a, interpolate(myBox.a[a], myBox.b[a], position)));
					const Real area = meshSurfaceArea(+p);
					const Real score = abs(0.5 - area / myArea);
					if (score < bestSplitScore)
					{
						bestSplitScore = score;
						bestSplitPosition = position;
					}
				}
				const Real split = interpolate(myBox.a[a], myBox.b[a], bestSplitPosition);
				Holder<Mesh> m1 = mesh->copy();
				Holder<Mesh> m2 = mesh->copy();
				meshClip(+m1, clippingBox(myBox, a, split));
				meshClip(+m2, clippingBox(myBox, a, split, true));
				std::vector<Holder<Mesh>> result = meshChunkingImpl(std::move(m1), config);
				std::vector<Holder<Mesh>> r2 = meshChunkingImpl(std::move(m2), config);
				for (auto &it : r2)
					result.push_back(std::move(it));
				return result;
			}
			else
			{
				// no more splitting is required
				std::vector<Holder<Mesh>> result;
				result.push_back(std::move(mesh));
				return result;
			}
		}
	}

	Holder<PointerRange<Holder<Mesh>>> meshChunking(const Mesh *msh, const MeshChunkingConfig &config)
	{
		if (msh->facesCount() == 0)
			return {};
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh chunking requires triangles mesh");
		if (config.maxSurfaceArea <= 0)
			CAGE_THROW_ERROR(Exception, "mesh chunking requires positive maxSurfaceArea");

		auto m = msh->copy();
		meshConvertToIndexed(+m);
		return PointerRangeHolder<Holder<Mesh>>(meshChunkingImpl(std::move(m), config));
	}

	void meshGenerateNormals(Mesh *msh, const MeshGenerateNormalsConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "generating normals requires triangles mesh");

		CAGE_ASSERT(msh->type() == MeshTypeEnum::Triangles);

		meshConvertToIndexed(msh);
		MeshImpl *impl = (MeshImpl *)msh;
		std::vector<Vec3> ns;
		ns.resize(impl->verticesCount());
		const uint32 tris = numeric_cast<uint32>(impl->indices.size() / 3);
		for (uint32 tri = 0; tri < tris; tri++)
		{
			const uint32 *ids = impl->indices.data() + tri * 3;
			const Vec3 &a = impl->positions[ids[0]];
			const Vec3 &b = impl->positions[ids[1]];
			const Vec3 &c = impl->positions[ids[2]];
			const Triangle t = Triangle(a, b, c);
			const Vec3 n = t.normal() * t.area();
			ns[ids[0]] += n;
			ns[ids[1]] += n;
			ns[ids[2]] += n;
		}
		for (Vec3 &n : ns)
			n = lengthSquared(n) > 1e-7 ? normalize(n) : Vec3();
		std::swap(impl->normals, ns);
	}

	void meshGenerateTangents(Mesh *msh, const MeshGenerateTangentsConfig &config)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "generateTangents");
	}

	void meshGenerateTexture(const Mesh *msh, const MeshGenerateTextureConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (msh->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "generating texture requires triangles mesh");
		if (msh->uvs().empty())
			CAGE_THROW_ERROR(Exception, "generating texture requires uvs");

		const uint32 triCount = msh->facesCount();
		const Vec2 scale = Vec2(config.width, config.height);
		for (uint32 triIdx = 0; triIdx < triCount; triIdx++)
		{
			Vec3i idx;
			if (msh->indices().empty())
				idx = Vec3i(triIdx * 3 + 0, triIdx * 3 + 1, triIdx * 3 + 2);
			else
				idx = Vec3i(msh->index(triIdx * 3 + 0), msh->index(triIdx * 3 + 1), msh->index(triIdx * 3 + 2));
			const Vec2 vertUvs[3] = { msh->uv(idx[0]) * scale, msh->uv(idx[1]) * scale, msh->uv(idx[2]) * scale };
			Vec2i t0 = Vec2i(sint32(vertUvs[0][0].value), sint32(vertUvs[0][1].value));
			Vec2i t1 = Vec2i(sint32(vertUvs[1][0].value), sint32(vertUvs[1][1].value));
			Vec2i t2 = Vec2i(sint32(vertUvs[2][0].value), sint32(vertUvs[2][1].value));
			// inspired by https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
			if (t0[1] > t1[1])
				std::swap(t0, t1);
			if (t0[1] > t2[1])
				std::swap(t0, t2);
			if (t1[1] > t2[1])
				std::swap(t1, t2);
			const sint32 totalHeight = t2[1] - t0[1];
			const Real totalHeightInv = 1.f / totalHeight;
			for (sint32 i = 0; i <= totalHeight; i++)
			{
				const bool secondHalf = i > t1[1] - t0[1] || t1[1] == t0[1];
				const uint32 segmentHeight = secondHalf ? t2[1] - t1[1] : t1[1] - t0[1];
				const Real alpha = totalHeight ? i * totalHeightInv : 0;
				const Real beta = totalHeight ? Real(i - (secondHalf ? t1[1] - t0[1] : 0)) / segmentHeight : 0;
				Vec2i A = t0 + (t2 - t0) * alpha;
				Vec2i B = secondHalf ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
				if (A[0] > B[0])
					std::swap(A, B);
				const sint32 y = t0[1] + i;
				CAGE_ASSERT(y >= 0 && y < config.height);
				for (sint32 x = A[0]; x <= B[0]; x++)
				{
					CAGE_ASSERT(x >= 0 && x < config.width);
					const Vec2 uv = Vec2(x, y);
					const Vec2 b = barycoord(vertUvs[0], vertUvs[1], vertUvs[2], uv);
					config.generator(Vec2i(x, y), idx, Vec3(b, 1 - b[0] - b[1]));
				}
			}
		}
	}

	namespace
	{
		template<class T>
		struct Sampler
		{
			void operator()(const Image *src, Image *dst, Vec2 srcUv, Vec2i dstCoord)
			{
				//srcUv[1] = 1 - srcUv[1];
				Vec2 uv = srcUv * Vec2(src->resolution());
				Vec2i p = Vec2i(uv);
				p = clamp(p, Vec2i(), src->resolution() - 2);
				uv -= Vec2(p);
				uv = clamp(uv, 0, 1);
				T corners[4];
				src->get(p + Vec2i(0, 0), corners[0]);
				src->get(p + Vec2i(1, 0), corners[1]);
				src->get(p + Vec2i(0, 1), corners[2]);
				src->get(p + Vec2i(1, 1), corners[3]);
				T res = interpolate(
					interpolate(corners[0], corners[1], uv[0]),
					interpolate(corners[2], corners[3], uv[0]),
					uv[1]
				);
				//dst->set(dstCoord[0], dst->resolution()[1] - dstCoord[1] - 1, res);
				dst->set(dstCoord, res);
			}
		};

		void imageRemap(const Image *src, Image *dst, Vec2 srcUv, Vec2i dstCoord)
		{
			CAGE_ASSERT(src->channels() == dst->channels());
			switch (src->channels())
			{
			case 1:
				Sampler<Real>()(src, dst, srcUv, dstCoord);
				break;
			case 2:
				Sampler<Vec2>()(src, dst, srcUv, dstCoord);
				break;
			case 3:
				Sampler<Vec3>()(src, dst, srcUv, dstCoord);
				break;
			case 4:
				Sampler<Vec4>()(src, dst, srcUv, dstCoord);
				break;
			}
		}
	}

	Holder<PointerRange<Holder<Image>>> meshRetexture(const MeshRetextureConfig &config)
	{
		if (config.source->type() != MeshTypeEnum::Triangles || config.target->type() != MeshTypeEnum::Triangles)
			CAGE_THROW_ERROR(Exception, "mesh retexture requires triangles mesh");

		struct Generator
		{
			const Mesh *mesh = nullptr;

			struct Pix
			{
				Vec3 pos;
				Vec2i xy;
			};

			std::vector<Pix> pixels;

			Generator(const Mesh *mesh) : mesh(mesh)
			{}

			void generate(const Vec2i &xy, const Vec3i &ids, const Vec3 &weights)
			{
				Pix p;
				p.pos = mesh->positionAt(ids, weights);
				p.xy = xy;
				pixels.push_back(p);
			}
		};
		Generator generator = Generator(config.target);
		generator.pixels.reserve(config.resolution[0] * config.resolution[1]);

		{
			MeshGenerateTextureConfig cfg;
			cfg.generator.bind<Generator, &Generator::generate>(&generator);
			cfg.width = config.resolution[0];
			cfg.height = config.resolution[1];
			meshGenerateTexture(config.target, cfg);
		}

		struct MeshFinder
		{
			const Mesh *msh = nullptr;
			std::vector<Triangle> tris;
			Holder<SpatialStructure> spatialData = newSpatialStructure({});
			Real maxDist;

			MeshFinder(const Mesh *msh) : msh(msh)
			{
				const uint32 trisCnt = msh->indicesCount() / 3;
				tris.reserve(trisCnt);
				for (uint32 t = 0; t < trisCnt; t++)
				{
					const uint32 *ids = msh->indices().data() + t * 3;
					const Vec3 &a = msh->positions()[ids[0]];
					const Vec3 &b = msh->positions()[ids[1]];
					const Vec3 &c = msh->positions()[ids[2]];
					Triangle tr(a, b, c);
					spatialData->update(t, tr);
					tris.push_back(tr);
				}
				spatialData->rebuild();
			}

			Vec2 find(const Vec3 &pos)
			{
				Holder<SpatialQuery> query = newSpatialQuery(spatialData.share());
				query->intersection(Sphere(pos, maxDist));
				Real bestDist = Real::Infinity();
				uint32 bestIndex = m;
				Vec3 bestPoint;
				for (uint32 i : query->result())
				{
					const Triangle t = tris[i];
					const Vec3 p = closestPoint(pos, t);
					const Real dist = distanceSquared(pos, p);
					if (dist < bestDist)
					{
						bestDist = dist;
						bestIndex = i;
						bestPoint = p;
					}
				}
				if (bestIndex == m)
					return Vec2::Nan();
				const Vec3i ids = Vec3i(msh->index(bestIndex * 3 + 0), msh->index(bestIndex * 3 + 1), msh->index(bestIndex * 3 + 2));
				const Vec2 bc = barycoord(msh->position(ids[0]), msh->position(ids[1]), msh->position(ids[2]), bestPoint);
				const Vec3 weights = Vec3(bc[0], bc[1], 1 - bc[0] - bc[1]);
				return msh->uvAt(ids, weights);
			}
		};
		MeshFinder finder = MeshFinder(config.source);
		finder.maxDist = config.maxDistance;

		PointerRangeHolder<Holder<Image>> res;
		res.reserve(config.inputs.size());
		for (const Image *it : config.inputs)
		{
			Holder<Image> i = newImage();
			i->initialize(config.resolution, it->channels(), it->format());
			res.push_back(std::move(i));
		}

		struct Tasker
		{
			PointerRange<const Image *> src;
			PointerRange<Holder<Image>> dst;
			Generator &generator;
			MeshFinder &finder;

			Tasker(PointerRange<const Image *> src, PointerRange<Holder<Image>> dst, Generator &generator, MeshFinder &finder) : src(src), dst(dst), generator(generator), finder(finder)
			{}

			void operator() (uint32 idx)
			{
				const Vec2 uv = finder.find(generator.pixels[idx].pos);
				if (!valid(uv))
					return;
				const Vec2i xy = generator.pixels[idx].xy;
				auto s = src.begin();
				for (auto &d : dst)
					imageRemap(*s++, +d, uv, xy);
			}
		};
		Tasker tasker = Tasker(config.inputs, res, generator, finder);

		if (config.parallelize)
			tasksRunBlocking<Tasker>("meshRetexture", tasker, generator.pixels.size());
		else
		{
			const uintPtr cnt = generator.pixels.size();
			for (uintPtr i = 0; i < cnt; i++)
				tasker(i);
		}

		return res;
	}
}
