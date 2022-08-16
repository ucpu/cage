#ifndef guard_meshExport_h_n1bv859wrffcg89
#define guard_meshExport_h_n1bv859wrffcg89

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API MeshExportTexture
	{
		String filename;
		const Image *image = nullptr;
	};

	struct CAGE_CORE_API MeshExportConfig
	{
		String name;
		MeshExportTexture albedo, pbr, normal;
		// the pbr texture contains: R: unused, G: roughness and B: metallic channels
		const Mesh *mesh = nullptr;
	};

	CAGE_CORE_API Holder<PointerRange<char>> meshExportBuffer(const MeshExportConfig &config);
	CAGE_CORE_API void meshExportFiles(const String &filename, const MeshExportConfig &config);
}

#endif // guard_meshExport_h_n1bv859wrffcg89
