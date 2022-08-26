#ifndef guard_mesh_h_CA052442302D4C3BAA9293200603C20A
#define guard_mesh_h_CA052442302D4C3BAA9293200603C20A

#include "math.h"

namespace cage
{
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

	CAGE_CORE_API void meshConvertToIndexed(Mesh *msh);
	CAGE_CORE_API void meshConvertToExpanded(Mesh *msh);

	CAGE_CORE_API void meshApplyTransform(Mesh *msh, const Transform &t);
	CAGE_CORE_API void meshApplyTransform(Mesh *msh, const Mat4 &t);

	CAGE_CORE_API void meshApplyAnimation(Mesh *msh, PointerRange<const Mat4> skinTransformation);

	CAGE_CORE_API void meshFlipNormals(Mesh *msh);

	CAGE_CORE_API void meshClip(Mesh *msh, const Aabb &box);
	CAGE_CORE_API void meshClip(Mesh *msh, const Plane &pln);
	CAGE_CORE_API Holder<Mesh> meshCut(Mesh *msh, const Plane &pln);

	CAGE_CORE_API void meshDiscardInvalid(Mesh *msh);
	CAGE_CORE_API void meshDiscardDisconnected(Mesh *msh);
	CAGE_CORE_API Holder<PointerRange<Holder<Mesh>>> meshSeparateDisconnected(const Mesh *msh);

	struct CAGE_CORE_API MeshMergeCloseVerticesConfig
	{
		Real distanceThreshold = 1e-5;
		bool moveVerticesOnly = false; // true -> vertices are moved only (no deduplication) and other attributes are ignored; false -> indices are remapped to other vertices
	};

	CAGE_CORE_API void meshMergeCloseVertices(Mesh *msh, const MeshMergeCloseVerticesConfig &config);

	struct CAGE_CORE_API MeshSimplifyConfig
	{
		Real minEdgeLength = 0.1;
		Real maxEdgeLength = 10;
		Real approximateError = 0.05;
		uint32 iterations = 10;
		bool useProjection = true;
	};

	CAGE_CORE_API void meshSimplify(Mesh *msh, const MeshSimplifyConfig &config);

	struct CAGE_CORE_API MeshRegularizeConfig
	{
		Real targetEdgeLength = 1;
		uint32 iterations = 10;
		bool useProjection = true;
	};

	CAGE_CORE_API void meshRegularize(Mesh *msh, const MeshRegularizeConfig &config);

	struct CAGE_CORE_API MeshChunkingConfig
	{
		Real maxSurfaceArea;
	};

	CAGE_CORE_API Holder<PointerRange<Holder<Mesh>>> meshChunking(const Mesh *msh, const MeshChunkingConfig &config);

	struct CAGE_CORE_API MeshUnwrapConfig
	{
		// charting determines which triangles will form islands
		uint32 maxChartIterations = 1;
		Real maxChartArea = 0;
		Real maxChartBoundaryLength = 0;
		Real chartNormalDeviation = 2.0;
		Real chartRoundness = 0.01;
		Real chartStraightness = 6.0;
		Real chartNormalSeam = 4.0;
		Real chartTextureSeam = 0.5;
		Real chartGrowThreshold = 2.0;

		// islands packing
		uint32 padding = 2;
		uint32 targetResolution = 0;
		Real texelsPerUnit = 0;

		bool logProgress = false;
	};

	CAGE_CORE_API uint32 meshUnwrap(Mesh *msh, const MeshUnwrapConfig &config);

	struct CAGE_CORE_API MeshGenerateTextureConfig
	{
		Delegate<void(const Vec2i &xy, const Vec3i &ids, const Vec3 &weights)> generator;
		uint32 width = 0;
		uint32 height = 0;
	};

	CAGE_CORE_API void meshGenerateTexture(const Mesh *msh, const MeshGenerateTextureConfig &config);

	struct CAGE_CORE_API MeshGenerateNormalsConfig
	{};

	CAGE_CORE_API void meshGenerateNormals(Mesh *msh, const MeshGenerateNormalsConfig &config);

	struct CAGE_CORE_API MeshGenerateTangentsConfig
	{};

	CAGE_CORE_API void meshGenerateTangents(Mesh *msh, const MeshGenerateTangentsConfig &config);

	struct CAGE_CORE_API MeshRetextureConfig
	{
		PointerRange<const Image *> inputs;
		const Mesh *source = nullptr;
		const Mesh *target = nullptr;
		Vec2i resolution;
		Real maxDistance;
		bool parallelize = false;
	};

	CAGE_CORE_API Holder<PointerRange<Holder<Image>>> meshRetexture(const MeshRetextureConfig &config);

	namespace detail
	{
		CAGE_CORE_API StringLiteral meshTypeToString(MeshTypeEnum type);
	}
}

#endif // guard_mesh_h_CA052442302D4C3BAA9293200603C20A
