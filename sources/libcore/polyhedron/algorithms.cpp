#include <cage-core/geometry.h>
#include <cage-core/macros.h>

#include "polyhedron.h"

#include <algorithm> // std::remove_if
#include <numeric> // std::iota

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

		void removeVertices(PolyhedronImpl *impl, const std::vector<bool> &verticesToRemove)
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

		void removeUnusedVertices(PolyhedronImpl *impl)
		{
			if (impl->indices.empty())
				return;
			std::vector<bool> verticesToRemove;
			verticesToRemove.resize(impl->positions.size(), true);
			for (uint32 i : impl->indices)
				verticesToRemove[i] = false;
			removeVertices(impl, verticesToRemove);
		}

		uint32 clipAddPoint(PolyhedronImpl *impl, uint32 ai, uint32 bi, uint32 axis, real value)
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

		void clipTriangles(PolyhedronImpl *impl, const std::vector<uint32> &in, std::vector<uint32> &out, uint32 axis, real value, bool side)
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
				uint32 m = as + bs + cs;
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
				while (as || (as == bs))
				{
					turnLeft(ids[0], ids[1], ids[2]);
					turnLeft(a, b, c);
					turnLeft(as, bs, cs);
				}
				CAGE_ASSERT(!as && bs);
				uint32 pi = clipAddPoint(impl, ids[0], ids[1], axis, value);
				if (m == 1)
				{
					CAGE_ASSERT(!cs);
					/*
					*         |
					* a +-------+ b
					*    \    |/
					*     \   /
					*      \ /|
					*     c + |
					*/
					uint32 qi = clipAddPoint(impl, ids[1], ids[2], axis, value);
					if (side)
					{
						out.push_back(ids[0]);
						out.push_back(pi);
						out.push_back(qi);

						out.push_back(ids[0]);
						out.push_back(qi);
						out.push_back(ids[2]);
					}
					else
					{
						out.push_back(pi);
						out.push_back(ids[1]);
						out.push_back(qi);
					}
				}
				else if (m == 2)
				{
					CAGE_ASSERT(cs);
					/*
					*     |
					* a +-------+ b
					*    \|    /
					*     \   /
					*     |\ /
					*     | + c
					*/
					uint32 qi = clipAddPoint(impl, ids[0], ids[2], axis, value);
					if (side)
					{
						out.push_back(ids[0]);
						out.push_back(pi);
						out.push_back(qi);
					}
					else
					{
						out.push_back(pi);
						out.push_back(ids[1]);
						out.push_back(qi);

						out.push_back(qi);
						out.push_back(ids[1]);
						out.push_back(ids[2]);
					}
				}
				else
				{
					CAGE_ASSERT(false);
				}
			}
		}

		// mark entire triplet (triangles) or pair (lines) if any one of them is marked
		void markFacesWithInvalidVertices(PolyhedronTypeEnum type, std::vector<bool> &marks)
		{
			switch (type)
			{
			case PolyhedronTypeEnum::Points:
				break;
			case PolyhedronTypeEnum::Lines:
			{
				const uint32 cnt = numeric_cast<uint32>(marks.size()) / 2;
				for (uint32 i = 0; i < cnt; i++)
				{
					if (marks[i * 2 + 0] || marks[i * 2 + 1])
						marks[i * 2 + 0] = marks[i * 2 + 1] = true;
				}
			} break;
			case PolyhedronTypeEnum::Triangles:
			{
				const uint32 cnt = numeric_cast<uint32>(marks.size()) / 3;
				for (uint32 i = 0; i < cnt; i++)
				{
					if (marks[i * 3 + 0] || marks[i * 3 + 1] || marks[i * 3 + 2])
						marks[i * 3 + 0] = marks[i * 3 + 1] = marks[i * 3 + 2] = true;
				}
			} break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid polyhedron type");
			}
		}

		void discardInvalidVertices(PolyhedronImpl *impl)
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
					*r = *r || !valid(v) || abs(lengthSquared(v) - 1) > 1e-5;
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

		void discardInvalidLines(PolyhedronImpl *impl)
		{
			// todo
		}

		void discardInvalidTriangles(PolyhedronImpl *impl)
		{
			const uint32 tris = impl->facesCount();
			PointerRange<const vec3> ps = impl->positions;
			if (impl->indices.empty())
			{
				std::vector<bool> verticesToRemove;
				verticesToRemove.reserve(tris * 3);
				for (uint32 i = 0; i < tris; i++)
				{
					triangle t(ps[i * 3 + 0], ps[i * 3 + 1], ps[i * 3 + 2]);
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
					triangle t(ps[is[i * 3 + 0]], ps[is[i * 3 + 1]], ps[is[i * 3 + 2]]);
					bool d = t.degenerated();
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

	void Polyhedron::convertToIndexed()
	{
		if (!indices().empty() || positions().empty())
			return;
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "convertToIndexed");
	}

	void Polyhedron::convertToExpanded()
	{
		if (indices().empty())
			return;
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "convertToExpanded");
	}

	void Polyhedron::mergeCloseVertices(real epsilon)
	{
		if (facesCount() == 0)
			return;

		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		const uint32 vc = numeric_cast<uint32>(impl->positions.size());
		std::vector<bool> needsRecenter; // mark vertices that need to reposition
		needsRecenter.resize(vc, false);
		std::vector<uint32> remap; // remap[originalVertexIndex] = newVertexIndex
		remap.resize(vc);
		std::iota(remap.begin(), remap.end(), (uint32)0);
		const vec3 *ps = impl->positions.data();

		// find which vertices can be remapped to other vertices
		const real threashold = epsilon * epsilon;
		for (uint32 i = 0; i < vc; i++)
		{
			for (uint32 j = 0; j < i; j++)
			{
				if (distanceSquared(ps[i], ps[j]) < threashold)
				{
					remap[i] = remap[j];
					needsRecenter[remap[i]] = true;
					break;
				}
			}
		}

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

		// remove faces that has collapsed
		discardInvalid();
	}

	void Polyhedron::generateTexture(const PolyhedronTextureGenerationConfig &config) const
	{
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Triangles);
		CAGE_ASSERT(!uvs().empty());

		const uint32 triCount = facesCount();
		const vec2 scale = vec2(config.width - 1, config.height - 1);
		for (uint32 triIdx = 0; triIdx < triCount; triIdx++)
		{
			ivec3 idx;
			if (indices().empty())
				idx = ivec3(triIdx * 3 + 0, triIdx * 3 + 1, triIdx * 3 + 2);
			else
				idx = ivec3(index(triIdx * 3 + 0), index(triIdx * 3 + 1), index(triIdx * 3 + 2));
			const vec2 vertUvs[3] = { uv(idx[0]) * scale, uv(idx[1]) * scale, uv(idx[2]) * scale };
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

	void Polyhedron::generateNormals(const PolyhedronNormalsGenerationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "generateNormals");
	}

	void Polyhedron::generateTangents(const PolyhedronTangentsGenerationConfig &config)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "generateTangents");
	}

	void Polyhedron::applyTransform(const transform &t)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		for (vec3 &it : impl->positions)
			it = t * it;
		for (vec3 &it : impl->normals)
			it = t.orientation * it;
		for (vec3 &it : impl->tangents)
			it = t.orientation * it;
		for (vec3 &it : impl->bitangents)
			it = t.orientation * it;
	}

	void Polyhedron::applyTransform(const mat4 &t)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		for (vec3 &it : impl->positions)
			it = vec3(t * vec4(it, 1));
		for (vec3 &it : impl->normals)
			it = vec3(t * vec4(it, 0));
		for (vec3 &it : impl->tangents)
			it = vec3(t * vec4(it, 0));
		for (vec3 &it : impl->bitangents)
			it = vec3(t * vec4(it, 0));
	}

	void Polyhedron::clip(const aabb &clipBox)
	{
		convertToIndexed();
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
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
				if (!intersects(triangle(a, b, c), clipBox))
					continue; // triangle fully outside
			}
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
			clear();
		else
			removeUnusedVertices(impl);
	}

	void Polyhedron::clip(const plane &pln)
	{
		// todo optimized code without constructing the other polyhedron
		cut(pln);
	}

	Holder<Polyhedron> Polyhedron::cut(const plane &pln)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "cut");
	}

	void Polyhedron::discardInvalid()
	{
		CAGE_ASSERT(verticesCount() + 1); // assert that vertices are consistent
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		discardInvalidVertices(impl);
		switch (impl->type)
		{
		case PolyhedronTypeEnum::Points:
			break;
		case PolyhedronTypeEnum::Lines:
			discardInvalidLines(impl);
			break;
		case PolyhedronTypeEnum::Triangles:
			discardInvalidTriangles(impl);
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid polyhedron type");
		}
		CAGE_ASSERT(verticesCount() + 1); // assert that vertices are consistent
	}

	void Polyhedron::discardDisconnected()
	{
		if (facesCount() == 0)
			return;
		// todo optimized code without separateDisconnected
		auto vec = separateDisconnected();
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
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		PolyhedronImpl *src = (PolyhedronImpl *)vec[largestIndex].get();
		impl->swap(*src);
	}

	Holder<PointerRange<Holder<Polyhedron>>> Polyhedron::separateDisconnected() const
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "separateDisconnected");
	}
}
