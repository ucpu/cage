#ifndef guard_mesh_h_CA052442302D4C3BAA9293200603C20A
#define guard_mesh_h_CA052442302D4C3BAA9293200603C20A

#include "math.h"

namespace cage
{
	class Collider;

	enum class MeshTypeEnum : uint32
	{
		Points = 1,
		Lines = 2,
		Triangles = 3,
	};

	class CAGE_CORE_API Mesh : private Immovable
	{
	public:
		void clear();
		Holder<Mesh> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		void importCollider(const Collider *collider);

		Holder<PointerRange<char>> exportBuffer(const String &format = ".cagemesh") const;
		void exportFile(const String &filename) const;

		uint32 verticesCount() const;

		PointerRange<const Vec3> positions() const;
		PointerRange<Vec3> positions();
		void positions(const PointerRange<const Vec3> &values);
		Vec3 position(uint32 idx) const;
		void position(uint32 idx, const Vec3 &value);
		Vec3 positionAt(const Vec3i &ids, const Vec3 &weights) const;

		PointerRange<const Vec3> normals() const;
		PointerRange<Vec3> normals();
		void normals(const PointerRange<const Vec3> &values);
		Vec3 normal(uint32 idx) const;
		void normal(uint32 idx, const Vec3 &value);
		Vec3 normalAt(const Vec3i &ids, const Vec3 &weights) const;

		PointerRange<const Vec3> tangents() const;
		PointerRange<Vec3> tangents();
		void tangents(const PointerRange<const Vec3> &values);
		Vec3 tangent(uint32 idx) const;
		void tangent(uint32 idx, const Vec3 &value);
		Vec3 tangentAt(const Vec3i &ids, const Vec3 &weights) const;

		PointerRange<const Vec4i> boneIndices() const;
		PointerRange<Vec4i> boneIndices();
		void boneIndices(const PointerRange<const Vec4i> &values);
		Vec4i boneIndices(uint32 idx) const;
		void boneIndices(uint32 idx, const Vec4i &value);
		Vec4i boneIndicesAt(const Vec3i &ids, const Vec3 &weights) const;

		PointerRange<const Vec4> boneWeights() const;
		PointerRange<Vec4> boneWeights();
		void boneWeights(const PointerRange<const Vec4> &values);
		Vec4 boneWeights(uint32 idx) const;
		void boneWeights(uint32 idx, const Vec4 &value);
		Vec4 boneWeightsAt(const Vec3i &ids, const Vec3 &weights) const;

		PointerRange<const Vec2> uvs() const;
		PointerRange<Vec2> uvs();
		void uvs(const PointerRange<const Vec2> &values);
		Vec2 uv(uint32 idx) const;
		void uv(uint32 idx, const Vec2 &value);
		Vec2 uvAt(const Vec3i &ids, const Vec3 &weights) const;

		PointerRange<const Vec3> uvs3() const;
		PointerRange<Vec3> uvs3();
		void uvs3(const PointerRange<const Vec3> &values);
		Vec3 uv3(uint32 idx) const;
		void uv3(uint32 idx, const Vec3 &value);
		Vec3 uv3At(const Vec3i &ids, const Vec3 &weights) const;

		void addVertex(const Vec3 &position);
		void addVertex(const Vec3 &position, const Vec3 &normal);
		void addVertex(const Vec3 &position, const Vec2 &uv);
		void addVertex(const Vec3 &position, const Vec3 &normal, const Vec2 &uv);

		Aabb boundingBox() const;
		Sphere boundingSphere() const;

		uint32 indicesCount() const;
		uint32 facesCount() const;

		PointerRange<const uint32> indices() const;
		PointerRange<uint32> indices();
		void indices(const PointerRange<const uint32> &values);
		uint32 index(uint32 idx) const;
		void index(uint32 idx, uint32 value);

		MeshTypeEnum type() const;
		void type(MeshTypeEnum t);

		void addPoint(uint32 a);
		void addPoint(const Vec3 &p);
		void addLine(uint32 a, uint32 b);
		void addLine(const Line &l);
		void addTriangle(uint32 a, uint32 b, uint32 c);
		void addTriangle(const Triangle &t);
	};

	CAGE_CORE_API Holder<Mesh> newMesh();

	namespace detail
	{
		CAGE_CORE_API StringPointer meshTypeToString(MeshTypeEnum type);
	}
}

#endif // guard_mesh_h_CA052442302D4C3BAA9293200603C20A
