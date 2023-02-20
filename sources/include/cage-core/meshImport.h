#ifndef guard_meshImport_h_rtkusd4g4b78fd
#define guard_meshImport_h_rtkusd4g4b78fd

#include "meshIoCommon.h"
#include "geometry.h"
#include "imageImport.h"

namespace cage
{
	class SkeletonRig;
	class SkeletalAnimation;

	struct CAGE_CORE_API MeshImportMaterial
	{
		Vec4 albedoBase = Vec4(0); // albedo rgb is linear, and NOT alpha-premultiplied
		Vec4 specialBase = Vec4(0); // cage material: roughness, metallic, emission, mask
		Vec4 albedoMult = Vec4(1);
		Vec4 specialMult = Vec4(1);
	};

	enum class MeshImportTextureType : uint32
	{
		None = 0,
		Albedo,
		Special, // cage material texture: roughness, metallic, emission, mask
		Normal,
		Opacity,
		Roughness,
		Metallic,
		Emission,
		Specular,
		Shininess,
		AmbientOcclusion,
		Mask, // cage mask -> white = use albedo from texture, black = use albedo from instance
		Bump,
	};

	struct CAGE_CORE_API MeshImportTexture
	{
		String name;
		MeshImportTextureType type = MeshImportTextureType::None;
		ImageImportResult images;
	};

	struct CAGE_CORE_API MeshImportPart
	{
		String objectName;
		String materialName;
		String shaderName;
		MeshImportMaterial material;
		Aabb boundingBox;
		Holder<Mesh> mesh;
		Holder<PointerRange<MeshImportTexture>> textures;
		MeshRenderFlags renderFlags = MeshRenderFlags::None;
		sint32 renderLayer = 0;
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
		bool mergeParts = false; // merge compatible parts to reduce draw calls (ignores material names)
		bool generateNormals = false;
		bool renormalizeVectors = true;
		bool passInvalidVectors = true;
		bool trianglesOnly = false; // discard non-triangle faces
		bool verticalFlipUv = true;
		bool verbose = false;
	};

	CAGE_CORE_API MeshImportResult meshImportFiles(const String &filename, const MeshImportConfig &config = {});

	CAGE_CORE_API void meshImportLoadExternal(MeshImportResult &result);
	CAGE_CORE_API void meshImportNormalizeFormats(MeshImportResult &result);
	CAGE_CORE_API void meshImportConvertToCageFormats(MeshImportResult &result);

	namespace detail
	{
		CAGE_CORE_API StringPointer meshImportTextureTypeToString(MeshImportTextureType type);
	}
}

#endif // guard_meshImport_h_rtkusd4g4b78fd
