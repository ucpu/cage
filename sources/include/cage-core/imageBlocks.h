#ifndef guard_imageBlocks_h_dsfhg54drftjsdrfgv
#define guard_imageBlocks_h_dsfhg54drftjsdrfgv

#include "image.h"
#include "math.h"

namespace cage
{
	struct CAGE_CORE_API ImageAstcEncodeConfig
	{
		Vec2i tiling;
		sint32 quality = 98;
		bool normals = false;
	};

	CAGE_CORE_API Holder<PointerRange<char>> imageAstcEncode(const Image *image, const ImageAstcEncodeConfig &config);

	struct CAGE_CORE_API ImageAstcDecodeConfig
	{
		Vec2i tiling;
		Vec2i resolution;
		uint32 channels = 4;
		ImageFormatEnum format = ImageFormatEnum::U8;
	};

	CAGE_CORE_API Holder<Image> imageAstcDecode(PointerRange<const char> buffer, const ImageAstcDecodeConfig &config);

	struct CAGE_CORE_API ImageKtxEncodeConfig
	{
		bool normals = false; // treat inputs as normal map
	};

	CAGE_CORE_API Holder<PointerRange<char>> imageKtxEncode(PointerRange<const Image *> images, const ImageKtxEncodeConfig &config);

	CAGE_CORE_API Holder<PointerRange<Holder<Image>>> imageKtxDecode(PointerRange<const char> buffer);

	enum class ImageKtxTranscodeFormatEnum : uint32
	{
		None = 0,
		Bc1, // rgb 0.16 dxt1
		Bc3, // rgba 0.25 dxt5
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
		uint32 mipmapLevel = 0;
		uint32 cubeFace = 0;
		uint32 layer = 0;
	};

	CAGE_CORE_API Holder<PointerRange<ImageKtxTranscodeResult>> imageKtxTranscode(PointerRange<const char> buffer, const ImageKtxTranscodeConfig &config);

	CAGE_CORE_API Holder<PointerRange<ImageKtxTranscodeResult>> imageKtxTranscode(PointerRange<const Image *> images, const ImageKtxEncodeConfig &compression, const ImageKtxTranscodeConfig &transcode);

	CAGE_CORE_API Holder<PointerRange<char>> imageBc1Encode(const Image *image, const ImageKtxEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc3Encode(const Image *image, const ImageKtxEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc4Encode(const Image *image, const ImageKtxEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc5Encode(const Image *image, const ImageKtxEncodeConfig &config = {});
	CAGE_CORE_API Holder<PointerRange<char>> imageBc7Encode(const Image *image, const ImageKtxEncodeConfig &config = {});
	CAGE_CORE_API Holder<Image> imageBc1Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc2Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc3Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc4Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc5Decode(PointerRange<const char> buffer, const Vec2i &resolution);
	CAGE_CORE_API Holder<Image> imageBc7Decode(PointerRange<const char> buffer, const Vec2i &resolution);
}

#endif // guard_imageBlocks_h_dsfhg54drftjsdrfgv
