#ifndef guard_imageKtx_h_1se5r6h4dsdg
#define guard_imageKtx_h_1se5r6h4dsdg

#include "image.h"
#include "math.h"

namespace cage
{
	struct CAGE_CORE_API ImageKtxCompressionConfig
	{
		//bool cubemap = false; // every consecutive 6 images in the input are cube faces: +X, -X, +Y, -Y, +Z, -Z
		//bool mipmaps = false; // generate and compress mipmap levels
		bool normals = false; // treat inputs as normal map
	};

	CAGE_CORE_API Holder<PointerRange<char>> imageKtxCompress(PointerRange<const Image *> images, const ImageKtxCompressionConfig &config);

	CAGE_CORE_API Holder<PointerRange<Holder<Image>>> imageKtxDecompressRaw(PointerRange<const char> buffer);

	struct CAGE_CORE_API ImageKtxDecompressionResult
	{
		Holder<PointerRange<char>> data;
		Vec2i resolution;
		Vec2i blocks;
	};

	CAGE_CORE_API Holder<PointerRange<ImageKtxDecompressionResult>> imageKtxDecompressBlocks(PointerRange<const char> buffer);
}

#endif // guard_imageKtx_h_1se5r6h4dsdg
