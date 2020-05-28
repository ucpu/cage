#include <cage-core/polyhedron.h>

#include <vector>

namespace cage
{
	template<class T>
	struct PolyhedronAttribute : public std::vector<T>
	{};

	class PolyhedronImpl : public Polyhedron
	{
	public:
		PolyhedronAttribute<vec3> positions;
		PolyhedronAttribute<vec3> normals;
		PolyhedronAttribute<vec2> uvs;
		PolyhedronAttribute<vec3> uvs3;
		PolyhedronAttribute<vec3> tangents;
		PolyhedronAttribute<vec3> bitangents;
		PolyhedronAttribute<ivec4> boneIndices;
		PolyhedronAttribute<vec4> boneWeights;

		std::vector<uint32> indices;

		PolyhedronTypeEnum type = PolyhedronTypeEnum::Triangles;
	};
}

#define POLYHEDRON_ATTRIBUTES positions, normals, tangents, bitangents, uvs, uvs3, boneIndices, boneWeights
