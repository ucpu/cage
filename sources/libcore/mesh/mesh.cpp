#include <cage-core/geometry.h>
#include <cage-core/collider.h>
#include <cage-core/serialization.h>
#include <cage-core/macros.h>
#include "mesh.h"

namespace cage
{
	void MeshImpl::swap(MeshImpl &other)
	{
#define GCHL_GENERATE(NAME) std::swap(NAME, other.NAME);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE

		std::swap(indices, other.indices);
		std::swap(type, other.type);
	}

	void Mesh::clear()
	{
		MeshImpl *impl = (MeshImpl *)this;

#define GCHL_GENERATE(NAME) impl->NAME.clear();
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE

		impl->indices.clear();
		impl->type = MeshTypeEnum::Triangles;
	}

	uint32 Mesh::verticesCount() const
	{
		const MeshImpl *impl = (const MeshImpl *)this;
#define GCHL_GENERATE(NAME) CAGE_ASSERT(impl->NAME.empty() || impl->NAME.size() == impl->positions.size());
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		CAGE_ASSERT(impl->uvs.empty() || impl->uvs3.empty());
		return numeric_cast<uint32>(impl->positions.size());
	}

	void Mesh::addVertex(const vec3 &position)
	{
		verticesCount(); // validate vertices
		MeshImpl *impl = (MeshImpl *)this;
		if (impl->positions.empty())
		{
			impl->positions.push_back(position);
		}
		else
		{
#define GCHL_GENERATE(NAME) if (!impl->NAME.empty()) impl->NAME.push_back({});
			CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
			impl->positions[impl->positions.size() - 1] = position;
		}
	}

	void Mesh::addVertex(const vec3 &position, const vec3 &normal)
	{
		MeshImpl *impl = (MeshImpl *)this;
		if (impl->positions.empty())
		{
			impl->positions.push_back(position);
			impl->normals.push_back(normal);
		}
		else
		{
			addVertex(position);
			impl->normals[impl->normals.size() - 1] = normal;
		}
	}

	void Mesh::addVertex(const vec3 &position, const vec2 &uv)
	{
		MeshImpl *impl = (MeshImpl *)this;
		if (impl->positions.empty())
		{
			impl->positions.push_back(position);
			impl->uvs.push_back(uv);
		}
		else
		{
			addVertex(position);
			impl->uvs[impl->uvs.size() - 1] = uv;
		}
	}

	void Mesh::addVertex(const vec3 &position, const vec3 &normal, const vec2 &uv)
	{
		MeshImpl *impl = (MeshImpl *)this;
		if (impl->positions.empty())
		{
			impl->positions.push_back(position);
			impl->normals.push_back(normal);
			impl->uvs.push_back(uv);
		}
		else
		{
			addVertex(position);
			impl->normals[impl->normals.size() - 1] = normal;
			impl->uvs[impl->uvs.size() - 1] = uv;
		}
	}

	Aabb Mesh::boundingBox() const
	{
		Aabb result;
		for (const vec3 &it : positions())
			result += Aabb(it);
		return result;
	}

	Sphere Mesh::boundingSphere() const
	{
		return makeSphere(positions());
	}

	uint32 Mesh::indicesCount() const
	{
		const MeshImpl *impl = (const MeshImpl *)this;
		return numeric_cast<uint32>(impl->indices.size());
	}

	uint32 Mesh::facesCount() const
	{
		const uint32 i = (indices().empty() ? numeric_cast<uint32>(positions().size()) : numeric_cast<uint32>(indices().size()));
		switch (type())
		{
		case MeshTypeEnum::Points:
			return i;
		case MeshTypeEnum::Lines:
			CAGE_ASSERT(i % 2 == 0);
			return i / 2;
		case MeshTypeEnum::Triangles:
			CAGE_ASSERT(i % 3 == 0);
			return i / 3;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid mesh type");
		}
	}

	PointerRange<const uint32> Mesh::indices() const
	{
		const MeshImpl *impl = (const MeshImpl *)this;
		return impl->indices;
	}

	PointerRange<uint32> Mesh::indices()
	{
		MeshImpl *impl = (MeshImpl *)this;
		return impl->indices;
	}

	void Mesh::indices(const PointerRange<const uint32> &values)
	{
		MeshImpl *impl = (MeshImpl *)this;
		impl->indices.resize(values.size());
		detail::memcpy(impl->indices.data(), values.data(), values.size() * sizeof(uint32));
	}

	uint32 Mesh::index(uint32 idx) const
	{
		const MeshImpl *impl = (const MeshImpl *)this;
		return impl->indices[idx];
	}

	void Mesh::index(uint32 idx, uint32 value)
	{
		MeshImpl *impl = (MeshImpl *)this;
		impl->indices[idx] = value;
	}

	MeshTypeEnum Mesh::type() const
	{
		const MeshImpl *impl = (const MeshImpl *)this;
		return impl->type;
	}

	void Mesh::type(MeshTypeEnum t)
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(verticesCount() == 0);
		impl->type = t;
	}

	void Mesh::addPoint(uint32 a)
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(type() == MeshTypeEnum::Points);
		impl->indices.push_back(a);
	}

	void Mesh::addPoint(const vec3 &p)
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(type() == MeshTypeEnum::Points);
		meshConvertToExpanded(impl);
		addVertex(p);
	}

	void Mesh::addLine(uint32 a, uint32 b)
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(type() == MeshTypeEnum::Lines);
		CAGE_ASSERT(impl->indices.size() % 2 == 0);
		impl->indices.push_back(a);
		impl->indices.push_back(b);
	}

	void Mesh::addLine(const Line &l)
	{
		CAGE_ASSERT(l.isSegment());
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(type() == MeshTypeEnum::Lines);
		meshConvertToExpanded(impl);
		addVertex(l.a());
		addVertex(l.b());
	}

	void Mesh::addTriangle(uint32 a, uint32 b, uint32 c)
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(type() == MeshTypeEnum::Triangles);
		CAGE_ASSERT(impl->indices.size() % 3 == 0);
		impl->indices.push_back(a);
		impl->indices.push_back(b);
		impl->indices.push_back(c);
	}

	void Mesh::addTriangle(const Triangle &t)
	{
		MeshImpl *impl = (MeshImpl *)this;
		CAGE_ASSERT(type() == MeshTypeEnum::Triangles);
		meshConvertToExpanded(impl);
		addVertex(t[0]);
		addVertex(t[1]);
		addVertex(t[2]);
		if (!impl->normals.empty())
		{
			const uint32 start = numeric_cast<uint32>(impl->normals.size()) - 3;
			const vec3 n = t.normal();
			impl->normals[start + 0] = n;
			impl->normals[start + 1] = n;
			impl->normals[start + 2] = n;
		}
	}

	Holder<Mesh> Mesh::copy() const
	{
		const MeshImpl *impl = (const MeshImpl *)this;
		Holder<Mesh> result = newMesh();
		result->type(impl->type);

#define GCHL_GENERATE(NAME) result->NAME(impl->NAME);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE

		result->indices(impl->indices);
		return result;
	}

	void Mesh::importCollider(const Collider *collider)
	{
		clear();
		CAGE_ASSERT(sizeof(Triangle) == sizeof(vec3) * 3);
		positions(bufferCast<const vec3>(collider->triangles()));
	}

	Holder<Mesh> newMesh()
	{
		return systemMemory().createImpl<Mesh, MeshImpl>();
	}
}
