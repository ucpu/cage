#ifndef guard_imageAlgorithms_h_fgzhj4gbv1r
#define guard_imageAlgorithms_h_fgzhj4gbv1r

#include "image.h"

namespace cage
{
	CAGE_CORE_API void imageConvert(Image *img, uint32 channels);
	CAGE_CORE_API void imageConvert(Image *img, ImageFormatEnum format);
	CAGE_CORE_API void imageConvert(Image *img, GammaSpaceEnum gammaSpace);
	CAGE_CORE_API void imageConvert(Image *img, AlphaModeEnum alphaMode);

	CAGE_CORE_API void imageConvertHeigthToNormal(Image *img, const Real &strength);
	CAGE_CORE_API void imageConvertSpecularToSpecial(Image *img);
	CAGE_CORE_API void imageConvertGltfPbrToSpecial(Image *img);
	CAGE_CORE_API void imageConvertSpecialToGltfPbr(Image *img);

	// setting a value must match the number of channels
	CAGE_CORE_API void imageFill(Image *img, const Real &value);
	CAGE_CORE_API void imageFill(Image *img, const Vec2 &value);
	CAGE_CORE_API void imageFill(Image *img, const Vec3 &value);
	CAGE_CORE_API void imageFill(Image *img, const Vec4 &value);

	CAGE_CORE_API void imageFill(Image *img, uint32 channel, float value);
	CAGE_CORE_API void imageFill(Image *img, uint32 channel, const Real &value);

	CAGE_CORE_API void imageVerticalFlip(Image *img);
	CAGE_CORE_API void imageResize(Image *img, uint32 width, uint32 height, bool useColorConfig = true);
	CAGE_CORE_API void imageResize(Image *img, const Vec2i &resolution, bool useColorConfig = true);
	CAGE_CORE_API void imageBoxBlur(Image *img, uint32 radius, uint32 rounds = 1, bool useColorConfig = true);
	CAGE_CORE_API void imageDilation(Image *img, uint32 rounds, bool useNan = false);
	CAGE_CORE_API void imageInvertColors(Image *img, bool useColorConfig = true);
	CAGE_CORE_API void imageInvertChannel(Image *img, uint32 channelIndex);

	CAGE_CORE_API Holder<PointerRange<Holder<Image>>> imageChannelsSplit(const Image *img);
	CAGE_CORE_API Holder<Image> imageChannelsJoin(PointerRange<const Image *> channels);
	CAGE_CORE_API Holder<Image> imageChannelsJoin(PointerRange<const Holder<Image>> channels);

	// copies parts of an image into another image
	// if the target and source images are the same instance, the source area and target area cannot overlap
	// if the target image is not initialized and the targetX and targetY are both zero, it will be initialized with channels and format of the source image
	// if the images have different format, transferred pixels will be converted to the target format
	// both images must have same number of channels
	// colorConfig is ignored (except when initializing new image)
	CAGE_CORE_API void imageBlit(const Image *source, Image *target, uint32 sourceX, uint32 sourceY, uint32 targetX, uint32 targetY, uint32 width, uint32 height);
	CAGE_CORE_API void imageBlit(const Image *source, Image *target, const Vec2i &sourceOffset, const Vec2i &targetOffset, const Vec2i &resolution);

	namespace detail
	{
		CAGE_CORE_API void imageResize(const uint8 *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth, uint8 *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth, uint32 channels);

		CAGE_CORE_API void imageResize(const float *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth, float *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth, uint32 channels);
	}
}

#endif // guard_imageAlgorithms_h_fgzhj4gbv1r
