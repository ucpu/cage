#include <cage-core/geometry.h>
#include <cage-core/collider.h>
#include <cage-core/serialization.h>
#include <cage-core/macros.h>
#include "polyhedron.h"

namespace cage
{
	void PolyhedronImpl::swap(PolyhedronImpl &other)
	{
#define GCHL_GENERATE(NAME) std::swap(NAME, other.NAME);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE

		std::swap(indices, other.indices);
		std::swap(type, other.type);
	}

	void Polyhedron::clear()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;

#define GCHL_GENERATE(NAME) impl->NAME.clear();
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE

		impl->indices.clear();
		impl->type = PolyhedronTypeEnum::Triangles;
	}

	uint32 Polyhedron::verticesCount() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
#define GCHL_GENERATE(NAME) CAGE_ASSERT(impl->NAME.empty() || impl->NAME.size() == impl->positions.size());
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE
		CAGE_ASSERT(impl->uvs.empty() || impl->uvs3.empty());
		return numeric_cast<uint32>(impl->positions.size());
	}

	void Polyhedron::addVertex(const vec3 &position)
	{
		verticesCount(); // validate vertices
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
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

	void Polyhedron::addVertex(const vec3 &position, const vec3 &normal)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
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

	void Polyhedron::addVertex(const vec3 &position, const vec2 &uv)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
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

	void Polyhedron::addVertex(const vec3 &position, const vec3 &normal, const vec2 &uv)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
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

	aabb Polyhedron::boundingBox() const
	{
		aabb result;
		for (const vec3 &it : positions())
			result += aabb(it);
		return result;
	}

	uint32 Polyhedron::indicesCount() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		return numeric_cast<uint32>(impl->indices.size());
	}

	uint32 Polyhedron::facesCount() const
	{
		const uint32 i = (indices().empty() ? numeric_cast<uint32>(positions().size()) : numeric_cast<uint32>(indices().size()));
		switch (type())
		{
		case PolyhedronTypeEnum::Points:
			return i;
		case PolyhedronTypeEnum::Lines:
			CAGE_ASSERT(i % 2 == 0);
			return i / 2;
		case PolyhedronTypeEnum::Triangles:
			CAGE_ASSERT(i % 3 == 0);
			return i / 3;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid polyhedron type");
		}
	}

	PointerRange<const uint32> Polyhedron::indices() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		return impl->indices;
	}

	PointerRange<uint32> Polyhedron::indices()
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		return impl->indices;
	}

	void Polyhedron::indices(const PointerRange<const uint32> &values)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		impl->indices.resize(values.size());
		detail::memcpy(impl->indices.data(), values.data(), values.size() * sizeof(uint32));
	}

	uint32 Polyhedron::index(uint32 idx) const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		return impl->indices[idx];
	}

	void Polyhedron::index(uint32 idx, uint32 value)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		impl->indices[idx] = value;
	}

	PolyhedronTypeEnum Polyhedron::type() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		return impl->type;
	}

	void Polyhedron::type(PolyhedronTypeEnum t)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(verticesCount() == 0);
		impl->type = t;
	}

	void Polyhedron::addPoint(uint32 a)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Points);
		impl->indices.push_back(a);
	}

	void Polyhedron::addPoint(const vec3 &p)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Points);
		convertToExpanded();
		addVertex(p);
	}

	void Polyhedron::addLine(uint32 a, uint32 b)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Lines);
		CAGE_ASSERT(impl->indices.size() % 2 == 0);
		impl->indices.push_back(a);
		impl->indices.push_back(b);
	}

	void Polyhedron::addLine(const line &l)
	{
		CAGE_ASSERT(l.isSegment());
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Lines);
		convertToExpanded();
		addVertex(l.a());
		addVertex(l.b());
	}

	void Polyhedron::addTriangle(uint32 a, uint32 b, uint32 c)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Triangles);
		CAGE_ASSERT(impl->indices.size() % 3 == 0);
		impl->indices.push_back(a);
		impl->indices.push_back(b);
		impl->indices.push_back(c);
	}

	void Polyhedron::addTriangle(const triangle &t)
	{
		PolyhedronImpl *impl = (PolyhedronImpl *)this;
		CAGE_ASSERT(type() == PolyhedronTypeEnum::Triangles);
		convertToExpanded();
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

	Holder<Polyhedron> Polyhedron::copy() const
	{
		const PolyhedronImpl *impl = (const PolyhedronImpl *)this;
		Holder<Polyhedron> result = newPolyhedron();
		result->type(impl->type);

#define GCHL_GENERATE(NAME) result->NAME(impl->NAME);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, POLYHEDRON_ATTRIBUTES));
#undef GCHL_GENERATE

		result->indices(impl->indices);
		return result;
	}

	void Polyhedron::importCollider(const Collider *collider)
	{
		clear();
		CAGE_ASSERT(sizeof(triangle) == sizeof(vec3) * 3);
		positions(bufferCast<const vec3>(collider->triangles()));
	}

	Holder<Polyhedron> newPolyhedron()
	{
		return detail::systemArena().createImpl<Polyhedron, PolyhedronImpl>();
	}
}