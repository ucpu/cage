#ifndef guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
#define guard_image_h_681DF37FA76B4FA48C656E96AF90EE69

namespace cage
{
	class CAGE_API imageClass
	{
	public:
		void empty(uint32 w, uint32 h, uint32 c = 4, uint32 bpc = 1);

		// encode png
		memoryBuffer encodeBuffer();
		void encodeFile(const string &filename);

		// decode png
		void decodeBuffer(const memoryBuffer &buffer, uint32 channels = m, uint32 bpc = 1);
		void decodeMemory(const void *buffer, uintPtr size, uint32 channels = m, uint32 bpc = 1);
		void decodeFile(const string &filename, uint32 channels = m, uint32 bpc = 1);

		uint32 width() const;
		uint32 height() const;
		uint32 channels() const;
		uint32 bytesPerChannel() const;

		void *bufferData();
		uintPtr bufferSize() const;

		float value(uint32 x, uint32 y, uint32 c) const;
		void value(uint32 x, uint32 y, uint32 c, float v);

		// getting a value must match the number of channels
		real get1(uint32 x, uint32 y) const;
		vec2 get2(uint32 x, uint32 y) const;
		vec3 get3(uint32 x, uint32 y) const;
		vec4 get4(uint32 x, uint32 y) const;

		// setting a value must match the number of channels
		void set(uint32 x, uint32 y, const real &value);
		void set(uint32 x, uint32 y, const vec2 &value);
		void set(uint32 x, uint32 y, const vec3 &value);
		void set(uint32 x, uint32 y, const vec4 &value);

		void verticalFlip();
		void convert(uint32 channels, uint32 bpc);
	};

	CAGE_API holder<imageClass> newImage();

	CAGE_API void imageBlit(imageClass *sourcePng, imageClass *targetPng, uint32 sourceX, uint32 sourceY, uint32 targetX, uint32 targetY, uint32 width, uint32 height);
}

#endif // guard_image_h_681DF37FA76B4FA48C656E96AF90EE69
