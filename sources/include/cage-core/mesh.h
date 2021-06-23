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

	struct CAGE_CORE_API MeshExportObjConfig
	{
		string materialLibraryName;
		string materialName;
		string objectName;
	};

	class CAGE_CORE_API Mesh : private Immovable
	{
	public:
		void clear();
		Holder<Mesh> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		void importCollider(const Collider *collider);

		Holder<PointerRange<char>> exportBuffer() const;
		Holder<PointerRange<char>> exportObjBuffer(const MeshExportObjConfig &config) const;
		void exportObjFile(const MeshExportObjConfig &config, const string &filename) const;

		uint32 verticesCount() const;

		PointerRange<const vec3> positions() const;
		PointerRange<vec3> positions();
		void positions(const PointerRange<const vec3> &values);
		vec3 position(uint32 idx) const;
		void position(uint32 idx, const vec3 &value);
		vec3 positionAt(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const vec3> normals() const;
		PointerRange<vec3> normals();
		void normals(const PointerRange<const vec3> &values);
		vec3 normal(uint32 idx) const;
		void normal(uint32 idx, const vec3 &value);
		vec3 normalAt(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const vec3> tangents() const;
		PointerRange<vec3> tangents();
		void tangents(const PointerRange<const vec3> &values);
		vec3 tangent(uint32 idx) const;
		void tangent(uint32 idx, const vec3 &value);
		vec3 tangentAt(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const vec3> bitangents() const;
		PointerRange<vec3> bitangents();
		void bitangents(const PointerRange<const vec3> &values);
		vec3 bitangent(uint32 idx) const;
		void bitangent(uint32 idx, const vec3 &value);
		vec3 bitangentAt(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const vec2> uvs() const;
		PointerRange<vec2> uvs();
		void uvs(const PointerRange<const vec2> &values);
		vec2 uv(uint32 idx) const;
		void uv(uint32 idx, const vec2 &value);
		vec2 uvAt(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const vec3> uvs3() const;
		PointerRange<vec3> uvs3();
		void uvs3(const PointerRange<const vec3> &values);
		vec3 uv3(uint32 idx) const;
		void uv3(uint32 idx, const vec3 &value);
		vec3 uv3At(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const ivec4> boneIndices() const;
		PointerRange<ivec4> boneIndices();
		void boneIndices(const PointerRange<const ivec4> &values);
		ivec4 boneIndices(uint32 idx) const;
		void boneIndices(uint32 idx, const ivec4 &value);
		ivec4 boneIndicesAt(const ivec3 &ids, const vec3 &weights) const;

		PointerRange<const vec4> boneWeights() const;
		PointerRange<vec4> boneWeights();
		void boneWeights(const PointerRange<const vec4> &values);
		vec4 boneWeights(uint32 idx) const;
		void boneWeights(uint32 idx, const vec4 &value);
		vec4 boneWeightsAt(const ivec3 &ids, const vec3 &weights) const;

		void addVertex(const vec3 &position);
		void addVertex(const vec3 &position, const vec3 &normal);
		void addVertex(const vec3 &position, const vec2 &uv);
		void addVertex(const vec3 &position, const vec3 &normal, const vec2 &uv);

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
		void addPoint(const vec3 &p);
		void addLine(uint32 a, uint32 b);
		void addLine(const Line &l);
		void addTriangle(uint32 a, uint32 b, uint32 c);
		void addTriangle(const Triangle &t);
	};

	CAGE_CORE_API Holder<Mesh> newMesh();

	CAGE_CORE_API void meshConvertToIndexed(Mesh *msh);
	CAGE_CORE_API void meshConvertToExpanded(Mesh *msh);

	CAGE_CORE_API void meshApplyTransform(Mesh *msh, const transform &t);
	CAGE_CORE_API void meshApplyTransform(Mesh *msh, const mat4 &t);

	CAGE_CORE_API void meshApplyAnimation(Mesh *msh, PointerRange<const mat4> skinTransformation);

	CAGE_CORE_API void meshFlipNormals(Mesh *msh);

	CAGE_CORE_API void meshClip(Mesh *msh, const Aabb &box);
	CAGE_CORE_API void meshClip(Mesh *msh, const Plane &pln);
	CAGE_CORE_API Holder<Mesh> meshCut(Mesh *msh, const Plane &pln);

	CAGE_CORE_API void meshDiscardInvalid(Mesh *msh);
	CAGE_CORE_API void meshDiscardDisconnected(Mesh *msh);
	CAGE_CORE_API Holder<PointerRange<Holder<Mesh>>> meshSeparateDisconnected(const Mesh *msh);

	struct CAGE_CORE_API MeshMergeCloseVerticesConfig
	{
		real distanceThreshold;
		bool moveVerticesOnly = false; // true -> vertices are moved only (no deduplication) and other attributes are ignored; false -> indices are remapped to other vertices
	};

	CAGE_CORE_API void meshMergeCloseVertices(Mesh *msh, const MeshMergeCloseVerticesConfig &config);

	struct CAGE_CORE_API MeshSimplifyConfig
	{
		real minEdgeLength = 0.1;
		real maxEdgeLength = 10;
		real approximateError = 0.05;
		uint32 iterations = 10;
		bool useProjection = true;
	};

	CAGE_CORE_API void meshSimplify(Mesh *msh, const MeshSimplifyConfig &config);

	struct CAGE_CORE_API MeshRegularizeConfig
	{
		real targetEdgeLength = 1;
		uint32 iterations = 10;
		bool useProjection = true;
	};

	CAGE_CORE_API void meshRegularize(Mesh *msh, const MeshRegularizeConfig &config);

	struct CAGE_CORE_API MeshChunkingConfig
	{
		real maxSurfaceArea;
	};

	CAGE_CORE_API Holder<PointerRange<Holder<Mesh>>> meshChunking(const Mesh *msh, const MeshChunkingConfig &config);

	struct CAGE_CORE_API MeshUnwrapConfig
	{
		// charting determines which triangles will form islands
		uint32 maxChartIterations = 1;
		real maxChartArea = 0;
		real maxChartBoundaryLength = 0;
		real chartNormalDeviation = 2.0;
		real chartRoundness = 0.01;
		real chartStraightness = 6.0;
		real chartNormalSeam = 4.0;
		real chartTextureSeam = 0.5;
		real chartGrowThreshold = 2.0;

		// islands packing
		uint32 padding = 2;
		uint32 targetResolution = 0;
		real texelsPerUnit = 0;

		bool logProgress = false;
	};

	CAGE_CORE_API uint32 meshUnwrap(Mesh *msh, const MeshUnwrapConfig &config);

	struct CAGE_CORE_API MeshGenerateTextureConfig
	{
		Delegate<void(uint32, uint32, const ivec3 &, const vec3 &)> generator;
		uint32 width = 0;
		uint32 height = 0;
	};

	CAGE_CORE_API void meshGenerateTexture(const Mesh *msh, const MeshGenerateTextureConfig &config);

	struct CAGE_CORE_API MeshGenerateNormalsConfig
	{
		// todo
	};

	CAGE_CORE_API void meshGenerateNormals(Mesh *msh, const MeshGenerateNormalsConfig &config);

	struct CAGE_CORE_API MeshGenerateTangentsConfig
	{
		// todo
	};

	CAGE_CORE_API void meshGenerateTangents(Mesh *msh, const MeshGenerateTangentsConfig &config);
}

#endif // guard_mesh_h_CA052442302D4C3BAA9293200603C20A
