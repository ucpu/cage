#ifndef guard_imageImport_h_our98esyd23gh4
#define guard_imageImport_h_our98esyd23gh4

#include <cage-core/image.h>
#include <cage-core/math.h>

namespace cage
{
	struct CAGE_CORE_API ImageImportRaw
	{
		detail::StringBase<64> format;
		Holder<PointerRange<char>> data;
		ImageColorConfig colorConfig;
		Vec2i resolution;
		uint32 channels = 0;
	};

	struct CAGE_CORE_API ImageImportPart
	{
		String fileName;
		Holder<Image> image;
		Holder<ImageImportRaw> raw;
		uint32 mipmapLevel = 0; // 0 = highest resolution
		uint32 cubeFace = 0; // +X, -X, +Y, -Y, +Z, -Z
		uint32 layer = 0; // index in an array or slice in 3D image
	};

	struct CAGE_CORE_API ImageImportResult
	{
		Holder<PointerRange<ImageImportPart>> parts;
	};

	struct CAGE_CORE_API ImageImportConfig
	{};

	CAGE_CORE_API ImageImportResult imageImportBuffer(PointerRange<const char> buffer, const ImageImportConfig &config = {});
	CAGE_CORE_API ImageImportResult imageImportFiles(const String &filesPattern, const ImageImportConfig &config = {});

	CAGE_CORE_API void imageImportConvertRawToImages(ImageImportResult &result);
	CAGE_CORE_API void imageImportConvertImagesToBcn(ImageImportResult &result, bool normals = false);
	CAGE_CORE_API void imageImportGenerateMipmaps(ImageImportResult &result);
}

#endif // guard_imageImport_h_our98esyd23gh4
