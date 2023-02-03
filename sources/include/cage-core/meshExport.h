#ifndef guard_meshExport_h_n1bv859wrffcg89
#define guard_meshExport_h_n1bv859wrffcg89

#include "mesh.h"

namespace cage
{
	class Image;

	struct CAGE_CORE_API MeshExportObjConfig
	{
		String materialLibraryName;
		String materialName;
		String objectName;
		const Mesh *mesh = nullptr;
		bool verticalFlipUv = true;
	};

	CAGE_CORE_API Holder<PointerRange<char>> meshExportBuffer(const MeshExportObjConfig &config);
	CAGE_CORE_API void meshExportFiles(const String &filename, const MeshExportObjConfig &config);

	struct CAGE_CORE_API MeshExportGltfTexture
	{
		String filename;
		const Image *image = nullptr;
	};

	struct CAGE_CORE_API MeshExportGltfConfig
	{
		String name;
		MeshExportGltfTexture albedo, pbr, normal;
		// the pbr texture contains: R: unused, G: roughness and B: metallic channels
		const Mesh *mesh = nullptr;
		bool verticalFlipUv = false;
		bool parallelize = false;
	};

	CAGE_CORE_API Holder<PointerRange<char>> meshExportBuffer(const MeshExportGltfConfig &config);
	CAGE_CORE_API void meshExportFiles(const String &filename, const MeshExportGltfConfig &config);
}

#endif // guard_meshExport_h_n1bv859wrffcg89
