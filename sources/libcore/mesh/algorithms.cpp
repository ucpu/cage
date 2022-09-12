#include <cage-core/geometry.h>
#include <cage-core/macros.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/image.h>
#include <cage-core/tasks.h>

#include "mesh.h"

#include <algorithm> // std::erase_if
#include <numeric> // std::iota

namespace cage
{
	namespace
	{
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

		template<class T>
		void turnLeft(T &a, T &b, T &c)
		{
			std::swap(a, b); // bac
			std::swap(b, c); // bca
		}

		void clipTriangles(MeshImpl *impl, const std::vector<uint32> &in, std::vector<uint32> &out, const uint32 axis, const Real value, const bool side)
		{
			CAGE_ASSERT((in.size() % 3) == 0);
			CAGE_ASSERT(out.size() == 0);
			const uint32 tris = numeric_cast<uint32>(in.size() / 3);
			for (uint32 tri = 0; tri < tris; tri++)
			{
				uint32 ids[3] = { in[tri * 3 + 0], in[tri * 3 + 1], in[tri * 3 + 2]};
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
					Triangle t(ps[i * 3 + 0], ps[i * 3 + 1], ps[i * 3 + 2]);
					bool d = t.degenerated();
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
					Triangle t(ps[is[i * 3 + 0]], ps[is[i * 3 + 1]], ps[is[i * 3 + 2]]);
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

		uint32 componentsRoot(std::vector<uint32> &c, uint32 a)
		{
			if (c[a] == a)
				return a;
			uint32 r = componentsRoot(c, c[a]);
			c[a] = r;
			return r;
		}

		void componentsMerge(std::vector<uint32> &c, uint32 a, uint32 b)
		{
			uint32 aa = componentsRoot(c, a), bb = componentsRoot(c, b);
			uint32 r = min(aa, bb);
			c[aa] = c[bb] = r;
			c[a] = c[b] = r;
		}

		Holder<PointerRange<Holder<Mesh>>> splitComponentsTriangles(const MeshImpl *src)
		{
			std::vector<uint32> components; // components[vertexIndex] = vertexIndex
			components.resize(src->positions.size());
			std::iota(components.begin(), components.end(), (uint32)0);

			const uint32 trisCnt = numeric_cast<uint32>(src->indices.size() / 3);
			for (uint32 t = 0; t < trisCnt; t++)
			{
				componentsMerge(components, src->indices[t * 3 + 0], src->indices[t * 3 + 1]);
				componentsMerge(components, src->indices[t * 3 + 1], src->indices[t * 3 + 2]);
			}

			const uint32 compsCount = numeric_cast<uint32>(components.size());
			for (uint32 i = 0; i < compsCount; i++)
				components[i] = componentsRoot(components, i);

			const uint32 indsCount = numeric_cast<uint32>(src->indices.size());
			std::vector<uint32> inds;
			inds.reserve(indsCount);
			PointerRangeHolder<Holder<Mesh>> result;
			for (uint32 c = 0; c < compsCount; c++)
			{
				if (components[c] != c)
					continue;
				Holder<Mesh> p = src->copy();
				for (uint32 i = 0; i < indsCount; i++)
				{
					if (components[src->index(i)] == c)
						inds.push_back(src->index(i));
				}
				p->indices(inds);
				removeUnusedVertices((MeshImpl *)p.get());
				inds.clear();
				result.push_back(std::move(p));
			}
			return result;
		}

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

	void meshConvertToIndexed(Mesh *msh)
	{
		if (!msh->indices().empty() || msh->positions().empty())
			return;
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "convertToIndexed");
	}

	void meshConvertToExpanded(Mesh *msh)
	{
		if (msh->indices().empty())
			return;
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "convertToExpanded");
	}

	void meshMergeCloseVertices(Mesh *msh, const MeshMergeCloseVerticesConfig &config)
	{
		if (msh->facesCount() == 0)
			return;

		if (!config.moveVerticesOnly)
			meshConvertToIndexed(msh);

		MeshImpl *impl = (MeshImpl *)msh;
		const uint32 vc = numeric_cast<uint32>(impl->positions.size());
		std::vector<bool> needsRecenter; // mark vertices that need to reposition
		needsRecenter.resize(vc, false);
		std::vector<uint32> remap; // remap[originalVertexIndex] = newVertexIndex
		remap.resize(vc);
		std::iota(remap.begin(), remap.end(), (uint32)0);
		PointerRange<const Vec3> ps = impl->positions;

		// find which vertices can be remapped to other vertices
		{
			const Real threashold = config.distanceThreshold * config.distanceThreshold;
			Holder<SpatialStructure> ss = newSpatialStructure({});
			for (uint32 i = 0; i < vc; i++)
				ss->update(i, ps[i]);
			ss->rebuild();
			Holder<SpatialQuery> q = newSpatialQuery(ss.share());
			for (uint32 i = 0; i < vc; i++)
			{
				q->intersection(Aabb(ps[i] - config.distanceThreshold, ps[i] + config.distanceThreshold));
				for (uint32 j : q->result())
				{
					if (distanceSquared(ps[i], ps[j]) < threashold)
					{
						remap[i] = remap[j];
						needsRecenter[remap[i]] = true;
						break;
					}
				}
			}
		}

		if (config.moveVerticesOnly)
		{
			// recenter vertices
			for (uint32 i = 0; i < vc; i++)
			{
				if (!needsRecenter[i])
					continue;
				Vec3 p;
				uint32 c = 0;
				for (uint32 j = i; j < vc; j++)
				{
					if (remap[j] == i)
					{
						p += ps[j];
						c++;
					}
				}
				if (c < 1)
					continue;
				p /= c;
				for (uint32 j = i; j < vc; j++)
				{
					if (remap[j] == i)
						impl->positions[j] = p;
				}
			}
		}
		else
		{
			// reassign indices
			for (uint32 &i : impl->indices)
				i = remap[i];
		}

		// remove faces that has collapsed
		meshDiscardInvalid(msh);
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
		const Vec2 scale = Vec2(config.width - 1, config.height - 1);
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
			for (sint32 i = 0; i < totalHeight; i++)
			{
				const bool secondHalf = i > t1[1] - t0[1] || t1[1] == t0[1];
				const uint32 segmentHeight = secondHalf ? t2[1] - t1[1] : t1[1] - t0[1];
				const Real alpha = i * totalHeightInv;
				const Real beta = Real(i - (secondHalf ? t1[1] - t0[1] : 0)) / segmentHeight;
				Vec2i A = t0 + (t2 - t0) * alpha;
				Vec2i B = secondHalf ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
				if (A[0] > B[0])
					std::swap(A, B);
				for (sint32 x = A[0]; x <= B[0]; x++)
				{
					const sint32 y = t0[1] + i;
					const Vec2 uv = Vec2(x, y);
					const Vec2 b = barycoord(vertUvs[0], vertUvs[1], vertUvs[2], uv);
					config.generator(Vec2i(x, y), idx, Vec3(b, 1 - b[0] - b[1]));
				}
			}
		}
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
			tasksRunBlocking<Tasker>("retexture", tasker, generator.pixels.size());
		else
		{
			const uintPtr cnt = generator.pixels.size();
			for (uintPtr i = 0; i < cnt; i++)
				tasker(i);
		}

		return res;
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
			n *= -1;
		// todo tangents & bitangents ?
	}

	void meshClip(Mesh *msh, const Aabb &clipBox)
	{
		// todo implement this as 6 times meshClip with a plane when it is implemented

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
		meshDiscardInvalid(impl);
		meshMergeCloseVertices(impl, {});
	}

	void meshClip(Mesh *msh, const Plane &pln)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "meshClip");
	}

	Holder<Mesh> meshCut(Mesh *msh, const Plane &pln)
	{
		Holder<Mesh> b = msh->copy();
		meshClip(msh, pln);
		meshClip(+b, Plane(pln.origin(), -pln.normal));
		return b;
	}

	void meshDiscardInvalid(Mesh *msh)
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

	void meshDiscardDisconnected(Mesh *msh)
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

	Holder<PointerRange<Holder<Mesh>>> meshSeparateDisconnected(const Mesh *msh)
	{
		msh->verticesCount(); // validate vertices
		if (msh->facesCount() == 0)
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
		case MeshTypeEnum::Points:
			CAGE_THROW_CRITICAL(NotImplemented, "separateDisconnected");
		case MeshTypeEnum::Lines:
			CAGE_THROW_CRITICAL(NotImplemented, "separateDisconnected");
		case MeshTypeEnum::Triangles:
			return splitComponentsTriangles(impl);
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid mesh type");
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

	void meshRemoveSmall(Mesh *msh, const MeshRemoveSmallConfig &config)
	{
		if (msh->facesCount() == 0)
			return;
		if (config.threshold <= 0)
			CAGE_THROW_ERROR(Exception, "mesh remove small requires positive threshold");

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
		meshDiscardInvalid(+msh);
	}
}
