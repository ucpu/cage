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

		Default = m, // used only for decoding an image, it will use the original format from the image
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
		void convert(uint32 channels);
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
