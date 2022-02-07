#ifndef guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
#define guard_image_h_681DF37FA76B4FA48C656E96AF90EE69

#include "core.h"

namespace cage
{
	enum class ImageFormatEnum : uint32
	{
		U8 = 1, // 8 bit normalized
		U16 = 2, // 16 bit normalized (native endianness)
		Rgbe = 3, // requires 3 channels exactly, each pixel encoded as one uint32
		Float = 4,

		Default = m, // used only for decoding an image, it will use original format from the file
	};

	enum class AlphaModeEnum : uint32
	{
		None = 0,
		Opacity,
		PremultipliedOpacity,
	};

	enum class GammaSpaceEnum : uint32
	{
		None = 0, // unknown
		Gamma,
		Linear,
	};

	struct CAGE_CORE_API ImageColorConfig
	{
		uint32 alphaChannelIndex = m; // index of the channel to use as a source of opacity (or first non-color channel)
		AlphaModeEnum alphaMode = AlphaModeEnum::None;
		GammaSpaceEnum gammaSpace = GammaSpaceEnum::None;
	};

	class CAGE_CORE_API Image : private Immovable
	{
	public:
		void clear();
		void initialize(uint32 width, uint32 height, uint32 channels = 4, ImageFormatEnum format = ImageFormatEnum::U8);
		void initialize(const Vec2i &resolution, uint32 channels = 4, ImageFormatEnum format = ImageFormatEnum::U8);
		Holder<Image> copy() const;

		// import from raw memory
		void importRaw(MemoryBuffer &&buffer, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format);
		void importRaw(MemoryBuffer &&buffer, const Vec2i &resolution, uint32 channels, ImageFormatEnum format);
		void importRaw(PointerRange<const char> buffer, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format);
		void importRaw(PointerRange<const char> buffer, const Vec2i &resolution, uint32 channels, ImageFormatEnum format);

		// image decode
		void importBuffer(PointerRange<const char> buffer, uint32 channels = m, ImageFormatEnum requestedFormat = ImageFormatEnum::Default);
		void importFile(const String &filename, uint32 channels = m, ImageFormatEnum requestedFormat = ImageFormatEnum::Default);

		// image encode
		Holder<PointerRange<char>> exportBuffer(const String &format = ".png") const;
		void exportFile(const String &filename) const;

		// the format must match
		// the image must outlive the view
		PointerRange<const uint8> rawViewU8() const;
		PointerRange<const uint16> rawViewU16() const;
		PointerRange<const float> rawViewFloat() const;

		uint32 width() const;
		uint32 height() const;
		Vec2i resolution() const;
		uint32 channels() const;
		ImageFormatEnum format() const;

		float value(uint32 x, uint32 y, uint32 c) const;
		float value(const Vec2i &position, uint32 c) const;
		void value(uint32 x, uint32 y, uint32 c, float v);
		void value(const Vec2i &position, uint32 c, float v);
		void value(uint32 x, uint32 y, uint32 c, const Real &v);
		void value(const Vec2i &position, uint32 c, const Real &v);

		// getting a value must match the number of channels
		Real get1(uint32 x, uint32 y) const;
		Vec2 get2(uint32 x, uint32 y) const;
		Vec3 get3(uint32 x, uint32 y) const;
		Vec4 get4(uint32 x, uint32 y) const;
		Real get1(const Vec2i &position) const;
		Vec2 get2(const Vec2i &position) const;
		Vec3 get3(const Vec2i &position) const;
		Vec4 get4(const Vec2i &position) const;
		void get(uint32 x, uint32 y, Real &value) const;
		void get(uint32 x, uint32 y, Vec2 &value) const;
		void get(uint32 x, uint32 y, Vec3 &value) const;
		void get(uint32 x, uint32 y, Vec4 &value) const;
		void get(const Vec2i &position, Real &value) const;
		void get(const Vec2i &position, Vec2 &value) const;
		void get(const Vec2i &position, Vec3 &value) const;
		void get(const Vec2i &position, Vec4 &value) const;

		// setting a value must match the number of channels
		void set(uint32 x, uint32 y, const Real &value);
		void set(uint32 x, uint32 y, const Vec2 &value);
		void set(uint32 x, uint32 y, const Vec3 &value);
		void set(uint32 x, uint32 y, const Vec4 &value);
		void set(const Vec2i &position, const Real &value);
		void set(const Vec2i &position, const Vec2 &value);
		void set(const Vec2i &position, const Vec3 &value);
		void set(const Vec2i &position, const Vec4 &value);

		ImageColorConfig colorConfig;
	};

	CAGE_CORE_API Holder<Image> newImage();

	CAGE_CORE_API void imageConvert(Image *img, uint32 channels);
	CAGE_CORE_API void imageConvert(Image *img, ImageFormatEnum format);
	CAGE_CORE_API void imageConvert(Image *img, GammaSpaceEnum gammaSpace);
	CAGE_CORE_API void imageConvert(Image *img, AlphaModeEnum alphaMode);

	CAGE_CORE_API void imageConvertHeigthToNormal(Image *img, const Real &strength);
	CAGE_CORE_API void imageConvertSpecularToSpecial(Image *img);

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
		CAGE_CORE_API void imageResize(
			const float *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth,
			      float *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth,
			uint32 channels);

		CAGE_CORE_API StringLiteral imageFormatToString(ImageFormatEnum format);
		CAGE_CORE_API StringLiteral imageGammaSpaceToString(GammaSpaceEnum space);
		CAGE_CORE_API StringLiteral imageAlphaModeToString(AlphaModeEnum mode);
	}
}

#endif // guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
