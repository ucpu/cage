#ifndef guard_imageImport_h_our98esyd23gh4
#define guard_imageImport_h_our98esyd23gh4

#include "math.h"

namespace cage
{
	struct CAGE_CORE_API ImageImportRaw
	{
		detail::StringBase<64> format;
		Holder<PointerRange<char>> data;
		Vec2i resolution;
		Vec2i blocks;
		uint32 channels = 0;
		bool normals = false;
	};

	struct CAGE_CORE_API ImageImportPart
	{
		String name;
		Holder<Image> image;
		Holder<ImageImportRaw> raw;
		Vec2i tile;
		double time = 0; // timestamp in an animation
		uint32 frameIndex = 0; // index in an animation
		uint32 mipmapLevel = 0; // 0 = highest resolution
		uint32 cubeFace = 0; // +X, -X, +Y, -Y, +Z, -Z
		uint32 layer = 0; // index in an array or slice in 3D image
		uint32 fileIndex = 0;
	};

	struct CAGE_CORE_API ImageImportResult
	{
		Holder<PointerRange<ImageImportPart>> parts;
	};

	struct CAGE_CORE_API ImageImportConfig
	{};

	CAGE_CORE_API ImageImportResult imageImportBuffer(PointerRange<const char> buffer, const ImageImportConfig &config = {});
	CAGE_CORE_API ImageImportResult imageImportFiles(const String &filesPattern, const ImageImportConfig &config = {});
}

#endif // guard_imageImport_h_our98esyd23gh4
