#ifndef guard_imageBlocks_h_dsfhg54drftjsdrfgv
#define guard_imageBlocks_h_dsfhg54drftjsdrfgv

#include <cage-core/image.h>
#include <cage-core/math.h>

namespace cage
{
	struct CAGE_CORE_API ImageBcnEncodeConfig
	{
		bool normals = false; // treat inputs as normal map
	};

	CAGE_CORE_API Holder<PointerRange<char>> imageBc1Encode(const Image *image, const ImageBcnEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc3Encode(const Image *image, const ImageBcnEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc4Encode(const Image *image, const ImageBcnEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc5Encode(const Image *image, const ImageBcnEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc7Encode(const Image *image, const ImageBcnEncodeConfig &config = {});
	CAGE_CORE_API Holder<Image> imageBc1Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc2Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc3Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc4Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc5Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc7Decode(PointerRange<const char> buffer, const Vec2i &resolution);
}

#endif // guard_imageBlocks_h_dsfhg54drftjsdrfgv
