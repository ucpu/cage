#ifndef guard_imageBlocksCompression_h_dsfhg54drftjsdrfgv
#define guard_imageBlocksCompression_h_dsfhg54drftjsdrfgv

#include "image.h"
#include "math.h"

namespace cage
{
	struct CAGE_CORE_API ImageAstcCompressionConfig
	{
		Vec2i tiling;
		sint32 quality = 98;
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

	struct CAGE_CORE_API ImageKtxCompressionConfig
	{
		//bool cubemap = false; // every consecutive 6 images in the input are cube faces: +X, -X, +Y, -Y, +Z, -Z
		//bool mipmaps = false; // generate and compress mipmap levels
		bool normals = false; // treat inputs as normal map
	};

	CAGE_CORE_API Holder<PointerRange<char>> imageKtxCompress(PointerRange<const Image *> images, const ImageKtxCompressionConfig &config);

	CAGE_CORE_API Holder<PointerRange<Holder<Image>>> imageKtxDecompress(PointerRange<const char> buffer);

	enum class ImageKtxTranscodeFormatEnum
	{
		None = 0,
		Bc1, // dxt1 rgb 0.16
		Bc3, // dxt5 rgba 0.25
		Bc4, // r 0.5
		Bc5, // rg 0.5
		Bc7, // rgba 0.25
		Astc, // rgba 0.25
	};

	struct CAGE_CORE_API ImageKtxTranscodeConfig
	{
		ImageKtxTranscodeFormatEnum format = ImageKtxTranscodeFormatEnum::None;
	};

	struct CAGE_CORE_API ImageKtxTranscodeResult
	{
		Holder<PointerRange<char>> data;
		Vec2i resolution;
		Vec2i blocks;
		uint32 mipmap = 0;
		uint32 face = 0;
		uint32 layer = 0;
	};

	CAGE_CORE_API Holder<PointerRange<ImageKtxTranscodeResult>> imageKtxTranscode(PointerRange<const char> buffer, const ImageKtxTranscodeConfig &config);

	CAGE_CORE_API Holder<PointerRange<ImageKtxTranscodeResult>> imageKtxTranscode(PointerRange<const Image *> images, const ImageKtxCompressionConfig &compression, const ImageKtxTranscodeConfig &transcode);
}

#endif // guard_imageBlocksCompression_h_dsfhg54drftjsdrfgv
