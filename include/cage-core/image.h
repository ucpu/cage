#ifndef guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
#define guard_image_h_681DF37FA76B4FA48C656E96AF90EE69

#include "core.h"

namespace cage
{
	// all formats except FloatLinear store the image in gamma space
	enum class ImageFormatEnum : uint32
	{
		U8 = 1, // 8 bit normalized
		U16 = 2, // 16 bit normalized (native endianness)
		Float,
		FloatLinear,
		Rgbe, // requires 3 channels exactly, each pixel encoded as one uint32

		Default = m, // used only for decoding an image, it will use the original format from the image
	};

	struct CAGE_CORE_API ImageOperationsConfig
	{
		// number of channels that will be converted to linear space before resizing and back to gamma space afterwards
		// set to 0 to skip all gamma conversions
		// this config is ignored if the format is FloatLinear
		uint32 gammaSpaceChannelsCount = 3;

		// index of a channel to use as alpha source, it will be premultiplied to all other channels before resizing and reverted afterwards
		// use -1 to ignore any alpha
		uint32 alphaChannelIndex = 3;

		float gamma = 2.2f;
	};

	class CAGE_CORE_API Image : private Immovable
	{
	public:
		void empty(uint32 width, uint32 height, uint32 channels = 4, ImageFormatEnum format = ImageFormatEnum::U8);

		// load from raw memory
		void loadBuffer(const MemoryBuffer &buffer, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format);
		void loadMemory(const void *buffer, uintPtr size, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format);

		// image decode
		void decodeBuffer(const MemoryBuffer &buffer, uint32 channels = m, ImageFormatEnum requestedFormat = ImageFormatEnum::Default);
		void decodeMemory(const void *buffer, uintPtr size, uint32 channels = m, ImageFormatEnum requestedFormat = ImageFormatEnum::Default);
		void decodeFile(const string &filename, uint32 channels = m, ImageFormatEnum requestedFormat = ImageFormatEnum::Default);

		// image encode
		MemoryBuffer encodeBuffer(const string &format = ".png");
		void encodeFile(const string &filename);

		uint32 width() const;
		uint32 height() const;
		uint32 channels() const;
		ImageFormatEnum format() const;

		float value(uint32 x, uint32 y, uint32 c) const;
		void value(uint32 x, uint32 y, uint32 c, float v);
		void value(uint32 x, uint32 y, uint32 c, const real &v);

		// getting a value must match the number of channels
		real get1(uint32 x, uint32 y) const;
		vec2 get2(uint32 x, uint32 y) const;
		vec3 get3(uint32 x, uint32 y) const;
		vec4 get4(uint32 x, uint32 y) const;
		void get(uint32 x, uint32 y, real &value) const;
		void get(uint32 x, uint32 y, vec2 &value) const;
		void get(uint32 x, uint32 y, vec3 &value) const;
		void get(uint32 x, uint32 y, vec4 &value) const;

		// setting a value must match the number of channels
		void set(uint32 x, uint32 y, const real &value);
		void set(uint32 x, uint32 y, const vec2 &value);
		void set(uint32 x, uint32 y, const vec3 &value);
		void set(uint32 x, uint32 y, const vec4 &value);

		// the format must match
		// the image must outlive the view
		PointerRange<const uint8> rawViewU8n() const;
		PointerRange<const float> rawViewFloat() const;

		void verticalFlip();

		void premultiplyAlpha(const ImageOperationsConfig &config = {});
		void demultiplyAlpha(const ImageOperationsConfig &config = {});

		void gammaToLinear(const ImageOperationsConfig &config = {});
		void linearToGamma(const ImageOperationsConfig &config = {});

		void resize(uint32 width, uint32 height, const ImageOperationsConfig &config = {});

		// add new channels (filled with zeros) or remove channels from the end
		// this conversion does not modify channels that are not added or removed
		void convert(uint32 channels);

		// conversions to and from FloatLinear applies corresponding gamma corrections
		// other conversions do not change the values other than encoding them in the desired format
		void convert(ImageFormatEnum format);
	};

	CAGE_CORE_API Holder<Image> newImage();

	// copies part of an image into another image
	// if the target and source images are the same instance, the source area and target area cannot overlap
	// if the target image is not initialized and the targetX and targetY are both zero, it will be initialized with channels and format of the source image
	// if the images have different format, transferred pixels will be converted to the target format
	// both images must have same number of channels
	CAGE_CORE_API void imageBlit(const Image *source, Image *target, uint32 sourceX, uint32 sourceY, uint32 targetX, uint32 targetY, uint32 width, uint32 height);
}

#endif // guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
