#ifndef guard_meshImport_h_rtkusd4g4b78fd
#define guard_meshImport_h_rtkusd4g4b78fd

#include "math.h"
#include "geometry.h"
#include "meshMaterial.h"
#include "imageImport.h"

namespace cage
{
	struct CAGE_CORE_API MeshImportTexture
	{
		String name;
		MeshTextureType type = MeshTextureType::None;
		ImageImportResult images;
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
		String rootPath = "/";
		String materialPathOverride; // enforce cage (cpm) material
		String materialNameAlternative; // prefer cage (cpm) material with this name if it exists
		detail::StringBase<6> axesSwizzle = "+x+y+z";
		Real scale = 1;
		bool mergeParts = false; // merge compatible parts to reduce draw calls (ignores material names)
		bool generateNormals = false;
		bool generateTangents = false;
		bool renormalizeVectors = true;
		bool passInvalidVectors = true;
		bool trianglesOnly = false; // discard non-triangle faces
		bool verbose = false;
	};

	CAGE_CORE_API MeshImportResult meshImportFiles(const String &filename, const MeshImportConfig &config = {});
}

#endif // guard_meshImport_h_rtkusd4g4b78fd
