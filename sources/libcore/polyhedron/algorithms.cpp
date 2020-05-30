#include <cage-core/geometry.h>

#include "polyhedron.h"

namespace cage
{
	void Polyhedron::convertToIndexed()
	{
		if (!indices().empty())
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
			const real invDenom = 1.0 / (d00 * d11 - d01 * d01);
			const real v = (d11 * d20 - d01 * d21) * invDenom;
			const real w = (d00 * d21 - d01 * d20) * invDenom;
			const real u = 1 - v - w;
			return vec2(u, v);
		}

		ivec2 operator * (const ivec2 &a, real b)
		{
			return ivec2(sint32(a[0] * b.value), sint32(a[1] * b.value));
		}
	}

	void Polyhedron::generateTexture(const PolyhedronTextureGenerationConfig &config) const
	{
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Triangles);
		CAGE_ASSERT(!uvs().empty());

		const uint32 triCount = (indices().empty() ? numeric_cast<uint32>(positions().size()) : numeric_cast<uint32>(indices().size())) / 3;
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
					CAGE_ASSERT(b.valid());
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
		// todo optimized code
		applyTransform(mat4(t));
	}

	void Polyhedron::applyTransform(const mat4 &t)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "applyTransform");
	}

	namespace
	{
		uint32 clipAddPoint(PolyhedronImpl *impl, uint32 ai, uint32 bi, uint32 axis, real value)
		{
			const vec3 a = impl->positions[ai];
			const vec3 b = impl->positions[bi];
			const real pu = (value - a[axis]) / (b[axis] - a[axis]);
			CAGE_ASSERT(pu >= 0 && pu <= 1);
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
		// todo remove unused vertices
	}

	void Polyhedron::clip(const plane &pln)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "clip");
	}

	Holder<Polyhedron> Polyhedron::cut(const plane &pln)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "cut");
	}

	void Polyhedron::discardDisconnected()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "discardDisconnected");
	}

	Holder<PointerRange<Holder<Polyhedron>>> Polyhedron::separateDisconnected()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_THROW_CRITICAL(NotImplemented, "separateDisconnected");
	}
}
