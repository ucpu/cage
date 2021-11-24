#ifndef guard_meshImport_h_rtkusd4g4b78fd
#define guard_meshImport_h_rtkusd4g4b78fd

#include "math.h"
#include "geometry.h"
#include "meshMaterial.h"

namespace cage
{
	struct CAGE_CORE_API MeshImportTexture
	{
		String name;
		MeshTextureType type = MeshTextureType::None;
		Holder<Image> image;
	};

	struct CAGE_CORE_API MeshImportPart
	{
		String objectName;
		String materialName;
		MeshMaterial material;
		Aabb boundingBox;
		MeshRenderFlags renderFlags = MeshRenderFlags::None;
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
