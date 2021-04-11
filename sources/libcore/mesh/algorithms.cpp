#include <cage-core/geometry.h>
#include <cage-core/macros.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/spatialStructure.h>

#include "mesh.h"

#include <algorithm> // std::remove_if
#include <numeric> // std::iota
#include <utility> // std::swap

namespace cage
{
	namespace
	{
		vec2 barycoord(const vec2 &a, const vec2 &b, const vec2 &c, const vec2 &p)
		{
			const vec2 v0 = b - a;
			const vec2 v1 = c - a;
			const vec2 v2 = p - a;
			const real d00 = dot(v0, v0);
			const real d01 = dot(v0, v1);
			const real d11 = dot(v1, v1);
			const real d20 = dot(v2, v0);
			const real d21 = dot(v2, v1);
			const real denom = d00 * d11 - d01 * d01;
			if (abs(denom) < 1e-7)
				return vec2(); // the triangle is too small or degenerated, just return anything valid
			const real invDenom = 1.0 / denom;
			const real v = (d11 * d20 - d01 * d21) * invDenom;
			const real w = (d00 * d21 - d01 * d20) * invDenom;
			const real u = 1 - v - w;
			CAGE_ASSERT(u.valid() && v.valid());
			return vec2(u, v);
		}

		ivec2 operator * (const ivec2 &a, real b)
		{
			return ivec2(sint32(a[0] * b.value), sint32(a[1] * b.value));
		}

		template<class T>
		void vectorEraseIf(std::vector<T> &v, const std::vector<bool> &toRemove)
		{
			CAGE_ASSERT(v.size() == toRemove.size());
			auto flagit = toRemove.begin();
			v.erase(std::remove_if(v.begin(), v.end(), [&](T&) {
				return *flagit++;
				}), v.end());
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

		uint32 clipAddPoint(MeshImpl *impl, const uint32 ai, const uint32 bi, const uint32 axis, const real value)
		{
			const vec3 a = impl->positions[ai];
			const vec3 b = impl->positions[bi];
			if (abs(b[axis] - a[axis]) < 1e-5)
				return ai; // the edge is very short or degenerated
			const real pu = (value - a[axis]) / (b[axis] - a[axis]);
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

		void clipTriangles(MeshImpl *impl, const std::vector<uint32> &in, std::vector<uint32> &out, const uint32 axis, const real value, const bool side)
		{
			CAGE_ASSERT((in.size() % 3) == 0);
			CAGE_ASSERT(out.size() == 0);
			const uint32 tris = numeric_cast<uint32>(in.size() / 3);
			for (uint32 tri = 0; tri < tris; tri++)
			{
				uint32 ids[3] = { in[tri * 3 + 0], in[tri * 3 + 1], in[tri * 3 + 2]};
				vec3 a = impl->positions[ids[0]];
				vec3 b = impl->positions[ids[1]];
				vec3 c = impl->positions[ids[2]];
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
				for (const vec3 &v : impl->positions)
				{
					*r = *r || !valid(v);
					r++;
				}
				CAGE_ASSERT(r == verticesToRemove.end());
			}
			{
				auto r = verticesToRemove.begin();
				for (const vec2 &v : impl->uvs)
				{
					*r = *r || !valid(v);
					r++;
				}
			}
			{
				auto r = verticesToRemove.begin();
				for (const vec3 &v : impl->uvs3)
				{
					*r = *r || !valid(v);
					r++;
				}
			}
			{
				auto r = verticesToRemove.begin();
				for (const vec3 &v : impl->normals)
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
			PointerRange<const vec3> ps = impl->positions;
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
				result.push_back(templates::move(p));
			}
			return result;
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

	void meshMergeCloseVertices(Mesh *msh, const MeshCloseVerticesMergingConfig &config)
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
		PointerRange<const vec3> ps = impl->positions;

		// find which vertices can be remapped to other vertices
		{
			const real threashold = config.distanceThreshold * config.distanceThreshold;
			Holder<SpatialStructure> ss = newSpatialStructure({});
			for (uint32 i = 0; i < vc; i++)
				ss->update(i, ps[i]);
			ss->rebuild();
			Holder<SpatialQuery> q = newSpatialQuery(+ss);
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
				vec3 p;
				uint32 c = 0;
				for (uint32 j = i; j < vc; j++)
				{
					if (remap[j] == i)
					{
						p += ps[j];
						c++;
					}
				}
				CAGE_ASSERT(c > 1);
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

	void meshGenerateTexture(const Mesh *msh, const MeshTextureGenerationConfig &config)
	{
		if (msh->facesCount() == 0)
			return;

		CAGE_ASSERT(msh->type() == MeshTypeEnum::Triangles);
		CAGE_ASSERT(!msh->uvs().empty());

		const uint32 triCount = msh->facesCount();
		const vec2 scale = vec2(config.width - 1, config.height - 1);
		for (uint32 triIdx = 0; triIdx < triCount; triIdx++)
		{
			ivec3 idx;
			if (msh->indices().empty())
				idx = ivec3(triIdx * 3 + 0, triIdx * 3 + 1, triIdx * 3 + 2);
			else
				idx = ivec3(msh->index(triIdx * 3 + 0), msh->index(triIdx * 3 + 1), msh->index(triIdx * 3 + 2));
			const vec2 vertUvs[3] = { msh->uv(idx[0]) * scale, msh->uv(idx[1]) * scale, msh->uv(idx[2]) * scale };
			ivec2 t0 = ivec2(sint32(vertUvs[0][0].value), sint32(vertUvs[0][1].value));
			ivec2 t1 = ivec2(sint32(vertUvs[1][0].value), sint32(vertUvs[1][1].value));
			ivec2 t2 = ivec2(sint32(vertUvs[2][0].value), sint32(vertUvs[2][1].value));
			// inspired by https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
			if (t0[1] > t1[1])
				std::swap(t0, t1);
			if (t0[1] > t2[1])
				std::swap(t0, t2);
			if (t1[1] > t2[1])
				std::swap(t1, t2);
			const sint32 totalHeight = t2[1] - t0[1];
			const real totalHeightInv = 1.f / totalHeight;
			for (sint32 i = 0; i < totalHeight; i++)
			{
				const bool secondHalf = i > t1[1] - t0[1] || t1[1] == t0[1];
				const uint32 segmentHeight = secondHalf ? t2[1] - t1[1] : t1[1] - t0[1];
				const real alpha = i * totalHeightInv;
				const real beta = real(i - (secondHalf ? t1[1] - t0[1] : 0)) / segmentHeight;
				ivec2 A = t0 + (t2 - t0) * alpha;
				ivec2 B = secondHalf ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
				if (A[0] > B[0])
					std::swap(A, B);
				for (sint32 x = A[0]; x <= B[0]; x++)
				{
					const sint32 y = t0[1] + i;
					const vec2 uv = vec2(x, y);
					const vec2 b = barycoord(vertUvs[0], vertUvs[1], vertUvs[2], uv);
					config.generator(x, y, idx, vec3(b, 1 - b[0] - b[1]));
				}
			}
		}
	}

	void meshGenerateNormals(Mesh *msh, const MeshNormalsGenerationConfig &config)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "generateNormals");
	}

	void meshGenerateTangents(Mesh *msh, const MeshTangentsGenerationConfig &config)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "generateTangents");
	}

	void meshApplyTransform(Mesh *msh, const transform &t)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		for (vec3 &it : impl->positions)
			it = t * it;
		for (vec3 &it : impl->normals)
			it = t.orientation * it;
		for (vec3 &it : impl->tangents)
			it = t.orientation * it;
		for (vec3 &it : impl->bitangents)
			it = t.orientation * it;
	}

	void meshApplyTransform(Mesh *msh, const mat4 &t)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		for (vec3 &it : impl->positions)
			it = vec3(t * vec4(it, 1));
		for (vec3 &it : impl->normals)
			it = vec3(t * vec4(it, 0));
		for (vec3 &it : impl->tangents)
			it = vec3(t * vec4(it, 0));
		for (vec3 &it : impl->bitangents)
			it = vec3(t * vec4(it, 0));
	}

	void meshApplyAnimation(Mesh *msh, PointerRange<const mat4> skinTransformation)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		const uint32 cnt = msh->verticesCount();
		CAGE_ASSERT(impl->boneIndices.size() == cnt);
		CAGE_ASSERT(impl->boneWeights.size() == cnt);
		for (uint32 i = 0; i < cnt; i++)
		{
			mat4 tr = mat4::scale(0);
			for (uint32 j = 0; j < 4; j++)
			{
				const uint32 b = impl->boneIndices[i][j];
				const real w = impl->boneWeights[i][j];
				tr += skinTransformation[b] * w;
			}
			CAGE_ASSERT(tr.valid());
			impl->positions[i] = vec3(tr * vec4(impl->positions[i], 1));
			mat3 tr3 = mat3(tr);
			if (!impl->normals.empty())
				impl->normals[i] = tr3 * impl->normals[i];
			if (!impl->tangents.empty())
				impl->tangents[i] = tr3 * impl->tangents[i];
			if (!impl->bitangents.empty())
				impl->bitangents[i] = tr3 * impl->bitangents[i];
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
		for (vec3 &n : impl->normals)
			n *= -1;
		// todo tangents & bitangents ?
	}

	void meshClip(Mesh *msh, const Aabb &clipBox)
	{
		if (msh->facesCount() == 0)
			return;

		CAGE_ASSERT(msh->type() == MeshTypeEnum::Triangles); // todo other types

		meshConvertToIndexed(msh);
		MeshImpl *impl = (MeshImpl *)msh;
		const vec3 clipBoxArr[2] = { clipBox.a, clipBox.b };
		std::vector<uint32> sourceIndices;
		sourceIndices.reserve(impl->indices.size());
		sourceIndices.swap(impl->indices);
		std::vector<uint32> tmp, tmp2;
		const uint32 tris = numeric_cast<uint32>(sourceIndices.size() / 3);
		for (uint32 tri = 0; tri < tris; tri++)
		{
			const uint32 *ids = sourceIndices.data() + tri * 3;
			{
				const vec3 &a = impl->positions[ids[0]];
				const vec3 &b = impl->positions[ids[1]];
				const vec3 &c = impl->positions[ids[2]];
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
	}

	void meshClip(Mesh *msh, const Plane &pln)
	{
		// todo optimized code without constructing the other mesh
		meshCut(msh, pln);
	}

	Holder<Mesh> meshCut(Mesh *msh, const Plane &pln)
	{
		MeshImpl *impl = (MeshImpl *)msh;
		CAGE_THROW_CRITICAL(NotImplemented, "cut");
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
}
