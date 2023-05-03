#include "mesh.h"

#include <cage-core/macros.h>

namespace cage
{
	template<class T>
	T attrAt(const MeshAttribute<T> &attribute, const Vec3i &ids, const Vec3 &weights)
	{
		return attribute.at(ids[0]) * weights[0] + attribute.at(ids[1]) * weights[1] + attribute.at(ids[2]) * weights[2];
	}

	template<>
	Vec4i attrAt<Vec4i>(const MeshAttribute<Vec4i> &attribute, const Vec3i &ids, const Vec3 &weights)
	{
		CAGE_THROW_ERROR(Exception, "cannot interpolate integer attributes");
	}

	template<class T, bool Normalize>
	T attrNormalize(const T &v)
	{
		return v;
	}

	template<>
	Vec3 attrNormalize<Vec3, true>(const Vec3 &v)
	{
		return normalize(v);
	}

#define GCHL_GENERATE(NORMALIZED, TYPE, SINGULAR, PLURAL) \
	PointerRange<const TYPE> Mesh::PLURAL() const \
	{ \
		const MeshImpl *impl = (const MeshImpl *)this; \
		return impl->PLURAL; \
	} \
	PointerRange<TYPE> Mesh::PLURAL() \
	{ \
		MeshImpl *impl = (MeshImpl *)this; \
		return impl->PLURAL; \
	} \
	void Mesh::PLURAL(const PointerRange<const TYPE> &values) \
	{ \
		MeshImpl *impl = (MeshImpl *)this; \
		impl->PLURAL.resize(values.size()); \
		if (!values.empty()) \
			detail::memcpy(impl->PLURAL.data(), values.data(), values.size() * sizeof(TYPE)); \
	} \
	TYPE Mesh::SINGULAR(uint32 idx) const \
	{ \
		const MeshImpl *impl = (const MeshImpl *)this; \
		return impl->PLURAL[idx]; \
	} \
	void Mesh::SINGULAR(uint32 idx, const TYPE &value) \
	{ \
		MeshImpl *impl = (MeshImpl *)this; \
		impl->PLURAL[idx] = value; \
	} \
	TYPE Mesh::CAGE_JOIN(SINGULAR, At)(const Vec3i &ids, const Vec3 &weights) const \
	{ \
		const MeshImpl *impl = (const MeshImpl *)this; \
		return attrNormalize<TYPE, NORMALIZED>(attrAt<TYPE>(impl->PLURAL, ids, weights)); \
	}

	GCHL_GENERATE(false, Vec3, position, positions);
	GCHL_GENERATE(true, Vec3, normal, normals);
	GCHL_GENERATE(false, Vec4i, boneIndices, boneIndices);
	GCHL_GENERATE(false, Vec4, boneWeights, boneWeights);
	GCHL_GENERATE(false, Vec2, uv, uvs);
	GCHL_GENERATE(false, Vec3, uv3, uvs3);
#undef GCHL_GENERATE
}
