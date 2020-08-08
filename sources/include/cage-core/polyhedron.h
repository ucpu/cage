#ifndef guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A
#define guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A

#include "math.h"

namespace cage
{
	enum class PolyhedronTypeEnum
	{
		Points = 1,
		Lines = 2,
		Triangles = 3,
	};

	struct CAGE_CORE_API PolyhedronSimplificationConfig
	{
		real minEdgeLength = 0.1;
		real maxEdgeLength = 10;
		real approximateError = 0.05;
		uint32 iterations = 10;
		bool useProjection = true;
	};

	struct CAGE_CORE_API PolyhedronRegularizationConfig
	{
		real targetEdgeLength = 1;
		uint32 iterations = 10;
		bool useProjection = true;
	};

	struct CAGE_CORE_API PolyhedronUnwrapConfig
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

	struct CAGE_CORE_API PolyhedronTextureGenerationConfig
	{
		Delegate<void(uint32, uint32, const ivec3 &, const vec3 &)> generator;
		uint32 width = 0;
		uint32 height = 0;
	};

	struct CAGE_CORE_API PolyhedronNormalsGenerationConfig
	{
		// todo
	};

	struct CAGE_CORE_API PolyhedronTangentsGenerationConfig
	{
		// todo
	};

	struct CAGE_CORE_API PolyhedronObjExportConfig
	{
		string materialLibraryName;
		string materialName;
		string objectName;
	};

	// aka a mesh but stored in cpu memory
	class CAGE_CORE_API Polyhedron : private Immovable
	{
	public:
		void clear();
		Holder<Polyhedron> copy() const;

		MemoryBuffer serialize() const;
		void deserialize(PointerRange<const char> buffer);

		void importCollider(const Collider *collider);

		MemoryBuffer exportObjBuffer(const PolyhedronObjExportConfig &config) const;
		void exportObjFile(const PolyhedronObjExportConfig &config, const string &filename) const;

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

		aabb boundingBox() const;

		uint32 indicesCount() const;
		uint32 facesCount() const;

		PointerRange<const uint32> indices() const;
		PointerRange<uint32> indices();
		void indices(const PointerRange<const uint32> &values);
		uint32 index(uint32 idx) const;
		void index(uint32 idx, uint32 value);

		PolyhedronTypeEnum type() const;
		void type(PolyhedronTypeEnum t);

		void addPoint(uint32 a);
		void addPoint(const vec3 &p);
		void addLine(uint32 a, uint32 b);
		void addLine(const line &l);
		void addTriangle(uint32 a, uint32 b, uint32 c);
		void addTriangle(const triangle &t);

		void convertToIndexed();
		void convertToExpanded();
		void mergeCloseVertices(real dist);
		void simplify(const PolyhedronSimplificationConfig &config);
		void regularize(const PolyhedronRegularizationConfig &config);
		uint32 unwrap(const PolyhedronUnwrapConfig &config);
		void generateTexture(const PolyhedronTextureGenerationConfig &config) const;
		void generateNormals(const PolyhedronNormalsGenerationConfig &config);
		void generateTangents(const PolyhedronTangentsGenerationConfig &config);
		void applyTransform(const transform &t);
		void applyTransform(const mat4 &t);
		void clip(const aabb &box);
		void clip(const plane &pln);
		Holder<Polyhedron> cut(const plane &pln);
		void discardInvalid();
		void discardDisconnected();
		Holder<PointerRange<Holder<Polyhedron>>> separateDisconnected() const;
	};

	CAGE_CORE_API Holder<Polyhedron> newPolyhedron();
}

#endif // guard_polyhedron_h_CA052442302D4C3BAA9293200603C20A