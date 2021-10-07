#ifndef guard_meshImport_h_rtkusd4g4b78fd
#define guard_meshImport_h_rtkusd4g4b78fd

#include "math.h"
#include "geometry.h"

namespace cage
{
	struct CAGE_CORE_API MeshImportMaterial
	{
		// albedo rgb is linear, and NOT alpha-premultiplied
		Vec4 albedoBase = Vec4(0);
		Vec4 specialBase = Vec4(0);
		Vec4 albedoMult = Vec4(1);
		Vec4 specialMult = Vec4(1);
	};

	enum class MeshImportRenderFlags : uint32
	{
		None = 0,
		Translucent = 1 << 1,
		TwoSided = 1 << 2,
		DepthTest = 1 << 3,
		DepthWrite = 1 << 4,
		VelocityWrite = 1 << 5,
		ShadowCast = 1 << 6,
		Lighting = 1 << 7,
	};
	GCHL_ENUM_BITS(MeshImportRenderFlags);

	enum class MeshImportTextureType : uint32
	{
		None = 0,
		Albedo,
		Special,
		Normal,
	};

	struct CAGE_CORE_API MeshImportTexture
	{
		String name;
		MeshImportTextureType type = MeshImportTextureType::None;
		//Holder<Image> image;
	};

	struct CAGE_CORE_API MeshImportPart
	{
		String objectName;
		String materialName;
		MeshImportMaterial material;
		Aabb boundingBox;
		MeshImportRenderFlags renderFlags = MeshImportRenderFlags::None;
		Holder<Mesh> mesh;
		Holder<PointerRange<MeshImportTexture>> textures;
	};

	struct CAGE_CORE_API MeshImportAnimation
	{
		String name;
		Holder<SkeletalAnimation> animation;
	};

	struct CAGE_CORE_API MeshImportResult
	{
		Holder<PointerRange<MeshImportPart>> parts;
		Holder<SkeletonRig> skeleton;
		Holder<PointerRange<MeshImportAnimation>> animations;
		Holder<PointerRange<String>> paths;
	};

	struct CAGE_CORE_API MeshImportConfig
	{
		String rootPath; // restrict file access to paths in this directory, leave empty to allow entire filesystem
		String materialPathOverride;
		String materialPathAlternative;
		detail::StringBase<6> axesSwizzle = "+x+y+z";
		Real scale = 1;
		bool bakeModel = false; // merge compatible parts to reduce draw calls (ignores material names)
		bool generateNormals = false;
		bool generateTangents = false;
		bool renormalizeVectors = true;
		bool passInvalidVectors = true;
		bool trianglesOnly = false; // discard non-triangle faces
		//bool loadExternalImages = false;
		bool verbose = false;
	};

	CAGE_CORE_API MeshImportResult meshImportFiles(const String &filename, const MeshImportConfig &config = {});
}

#endif // guard_meshImport_h_rtkusd4g4b78fd
