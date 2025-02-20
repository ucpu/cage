#ifndef guard_meshAlgorithms_h_df5641hj6fghj
#define guard_meshAlgorithms_h_df5641hj6fghj

#include <cage-core/mesh.h>

namespace cage
{
	class Image;

	CAGE_CORE_API void meshConvertToIndexed(Mesh *msh);
	CAGE_CORE_API void meshConvertToExpanded(Mesh *msh);

	CAGE_CORE_API void meshApplyTransform(Mesh *msh, const Transform &t);
	CAGE_CORE_API void meshApplyTransform(Mesh *msh, const Mat4 &t);

	CAGE_CORE_API void meshApplyAnimation(Mesh *msh, PointerRange<const Mat4> skinTransformation);

	CAGE_CORE_API void meshFlipNormals(Mesh *msh);
	CAGE_CORE_API void meshDuplicateSides(Mesh *msh);
	CAGE_CORE_API void meshConsistentWinding(Mesh *msh);

	struct CAGE_CORE_API MeshMergeCloseVerticesConfig
	{
		Real distanceThreshold = 1e-5;
		bool moveVerticesOnly = false; // true -> vertices are moved only (no deduplication) and other attributes are ignored; false -> indices are remapped to other vertices
	};
	CAGE_CORE_API void meshMergeCloseVertices(Mesh *msh, const MeshMergeCloseVerticesConfig &config);

	CAGE_CORE_API void meshMergePlanar(Mesh *msh);

	CAGE_CORE_API Holder<Mesh> meshCut(Mesh *msh, const Plane &pln);
	CAGE_CORE_API Holder<PointerRange<Holder<Mesh>>> meshSeparateDisconnected(const Mesh *msh);

	struct CAGE_CORE_API MeshSplitLongConfig
	{
		// triangle is split if it is longer than the specified length AND its "spikiness" is greater than the specified ratio
		Real length = 1; // using length = 0 disables checking for length -> only spiky triangles are split
		Real ratio = 1; // using ratio = 1 disables checking for spikiness -> only long triangles are split
	};
	CAGE_CORE_API void meshSplitLong(Mesh *msh, const MeshSplitLongConfig &config);

	struct CAGE_CORE_API MeshSplitIntersectingConfig
	{
		uint32 maxCuttersForTriangle = m;
		bool parallelize = false;
	};
	CAGE_CORE_API void meshSplitIntersecting(Mesh *msh, const MeshSplitIntersectingConfig &config);

	CAGE_CORE_API void meshClip(Mesh *msh, const Aabb &box);
	CAGE_CORE_API void meshClip(Mesh *msh, const Plane &pln);

	CAGE_CORE_API bool meshDetectInvalid(const Mesh *msh);
	CAGE_CORE_API void meshRemoveInvalid(Mesh *msh);
	CAGE_CORE_API void meshRemoveDisconnected(Mesh *msh);

	struct CAGE_CORE_API MeshRemoveSmallConfig
	{
		Real threshold = 0.1; // area for triangles or length for lines
	};
	CAGE_CORE_API void meshRemoveSmall(Mesh *msh, const MeshRemoveSmallConfig &config);

	struct CAGE_CORE_API MeshRemoveOccludedConfig
	{
		Real raysPerUnitArea = 1;
		uint32 minRaysPerTriangle = 10;
		uint32 maxRaysPerTriangle = 50;
		Rads grazingAngle = Degs(5);
		bool doubleSided = false;
		bool parallelize = false;
	};
	CAGE_CORE_API void meshRemoveOccluded(Mesh *msh, const MeshRemoveOccludedConfig &config);

	struct CAGE_CORE_API MeshSimplifyConfig
	{
		Real minEdgeLength = 0.1;
		Real maxEdgeLength = 10;
		Real approximateError = 0.05;
		uint32 iterations = 10;
		bool useProjection = true;
		bool ignoreInvalid = false;
	};
	CAGE_CORE_API void meshSimplify(Mesh *msh, const MeshSimplifyConfig &config);

	struct CAGE_CORE_API MeshDecimateConfig
	{
		uint32 targetVertices = 0;
		bool ignoreInvalid = false;
	};
	CAGE_CORE_API void meshDecimate(Mesh *msh, const MeshDecimateConfig &config);

	struct CAGE_CORE_API MeshRegularizeConfig
	{
		Real targetEdgeLength = 1;
		uint32 iterations = 10;
		bool useProjection = true;
		bool ignoreInvalid = false;
	};
	CAGE_CORE_API void meshRegularize(Mesh *msh, const MeshRegularizeConfig &config);

	struct CAGE_CORE_API MeshSmoothingConfig
	{
		uint32 iterations = 10;
		bool uniform = false;
		bool ignoreInvalid = false;
	};
	CAGE_CORE_API void meshSmoothing(Mesh *msh, const MeshSmoothingConfig &config);

	struct CAGE_CORE_API MeshChunkingConfig
	{
		Real maxSurfaceArea;
		bool parallelize = false;
	};
	CAGE_CORE_API Holder<PointerRange<Holder<Mesh>>> meshChunking(const Mesh *msh, const MeshChunkingConfig &config);

	struct CAGE_CORE_API MeshConvexHullConfig
	{};
	CAGE_CORE_API Holder<Mesh> meshConvexHull(const Mesh *msh, const MeshConvexHullConfig &config);

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

	struct CAGE_CORE_API MeshGenerateNormalsConfig
	{
		bool flat = false;
	};
	CAGE_CORE_API void meshGenerateNormals(Mesh *msh, const MeshGenerateNormalsConfig &config);

	struct CAGE_CORE_API MeshGenerateTextureConfig
	{
		Delegate<void(const Vec2i &xy, const Vec3i &ids, const Vec3 &weights)> generator;
		uint32 width = 0;
		uint32 height = 0;
	};
	CAGE_CORE_API void meshGenerateTexture(const Mesh *msh, const MeshGenerateTextureConfig &config);

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

	struct CAGE_CORE_API MeshMergeInput
	{
		Mesh *mesh = nullptr;
		PointerRange<const Image *> textures;
	};
	struct CAGE_CORE_API MeshMergeResult
	{
		Holder<Mesh> mesh;
		Holder<PointerRange<Holder<Image>>> textures;
	};
	CAGE_CORE_API MeshMergeResult meshMerge(PointerRange<const MeshMergeInput> inputs);
}

#endif // guard_meshAlgorithms_h_df5641hj6fghj
