#ifndef guard_imageAstc_h_dsfhg54drftjsdrfgv
#define guard_imageAstc_h_dsfhg54drftjsdrfgv

#include "image.h"
#include "math.h"

namespace cage
{
	namespace privat
	{
#ifdef CAGE_DEBUG
		constexpr sint32 AstcDefaultCompressionQuality = 10;
#else
		constexpr sint32 AstcDefaultCompressionQuality = 98;
#endif // CAGE_DEBUG
	}

	struct CAGE_CORE_API ImageAstcCompressionConfig
	{
		Vec2i tiling;
		sint32 quality = privat::AstcDefaultCompressionQuality;
		bool normals = false;
	};

	CAGE_CORE_API Holder<PointerRange<char>> imageAstcCompress(const Image *image, const ImageAstcCompressionConfig &config);

	struct CAGE_CORE_API ImageAstcDecompressionConfig
	{
		Vec2i tiling;
		Vec2i resolution;
		uint32 channels = 4;
		ImageFormatEnum format = ImageFormatEnum::U8;
	};

	CAGE_CORE_API Holder<Image> imageAstcDecompress(PointerRange<const char> buffer, const ImageAstcDecompressionConfig &config);
}

#endif // guard_imageAstc_h_dsfhg54drftjsdrfgv
