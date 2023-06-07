#ifndef guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
#define guard_image_h_681DF37FA76B4FA48C656E96AF90EE69

#include "core.h"

namespace cage
{
	struct MemoryBuffer;

	enum class ImageFormatEnum : uint32
	{
		U8 = 1, // 8 bit normalized
		U16 = 2, // 16 bit normalized (native endianness)
		Float = 4,

		Default = m, // used only for decoding an image, it will use original format from the file
	};
	CAGE_CORE_API StringPointer imageFormatToString(ImageFormatEnum format);

	enum class AlphaModeEnum : uint32
	{
		None = 0,
		Opacity,
		PremultipliedOpacity,
	};
	CAGE_CORE_API StringPointer imageAlphaModeToString(AlphaModeEnum mode);

	enum class GammaSpaceEnum : uint32
	{
		None = 0, // unknown
		Gamma,
		Linear,
	};
	CAGE_CORE_API StringPointer imageGammaSpaceToString(GammaSpaceEnum space);

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
}

#endif // guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
