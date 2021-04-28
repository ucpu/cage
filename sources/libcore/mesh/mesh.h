#include <cage-core/mesh.h>

#include <vector>

namespace cage
{
	template<class T>
	struct MeshAttribute : public std::vector<T>
	{};

	class MeshImpl : public Mesh
	{
	public:
		MeshAttribute<vec3> positions;
		MeshAttribute<vec3> normals;
		MeshAttribute<vec2> uvs;
		MeshAttribute<vec3> uvs3;
		MeshAttribute<vec3> tangents;
		MeshAttribute<vec3> bitangents;
		MeshAttribute<ivec4> boneIndices;
		MeshAttribute<vec4> boneWeights;

		std::vector<uint32> indices;

		MeshTypeEnum type = MeshTypeEnum::Triangles;

		void swap(MeshImpl &other);
	};
}

#define POLYHEDRON_ATTRIBUTES positions, normals, tangents, bitangents, uvs, uvs3, boneIndices, boneWeights
