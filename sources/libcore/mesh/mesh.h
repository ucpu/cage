#include <vector>

#include <cage-core/mesh.h>

namespace cage
{
	template<class T>
	struct MeshAttribute : public std::vector<T>
	{};

	class MeshImpl : public Mesh
	{
	public:
		MeshAttribute<Vec3> positions;
		MeshAttribute<Vec3> normals;
		MeshAttribute<Vec4i> boneIndices;
		MeshAttribute<Vec4> boneWeights;
		MeshAttribute<Vec2> uvs;
		MeshAttribute<Vec3> uvs3;

		std::vector<uint32> indices;

		MeshTypeEnum type = MeshTypeEnum::Triangles;

		void swap(MeshImpl &other);
	};
}

#define POLYHEDRON_ATTRIBUTES positions, normals, boneIndices, boneWeights, uvs, uvs3
