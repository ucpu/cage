#ifndef guard_png_h_681DF37FA76B4FA48C656E96AF90EE69
#define guard_png_h_681DF37FA76B4FA48C656E96AF90EE69

namespace cage
{
	class CAGE_API pngImageClass
	{
	public:
		void empty(uint32 w, uint32 h, uint32 c = 4, uint32 bpc = 1);
		memoryBuffer encodeBuffer();
		void encodeFile(const string &filename);

		void decodeBuffer(const memoryBuffer &buffer, uint32 channels = -1, uint32 bpc = 1);
		void decodeMemory(const void *buffer, uintPtr size, uint32 channels = -1, uint32 bpc = 1);
		void decodeFile(const string &filename, uint32 channels = -1, uint32 bpc = 1);

		uint32 width() const;
		uint32 height() const;
		uint32 channels() const;
		uint32 bytesPerChannel() const;

		void *bufferData();
		uintPtr bufferSize() const;

		float value(uint32 x, uint32 y, uint32 c) const;
		void value(uint32 x, uint32 y, uint32 c, float v);

		void verticalFlip();
		void convert(uint32 channels, uint32 bpc);
	};

	CAGE_API holder<pngImageClass> newPngImage();

	CAGE_API void pngBlit(pngImageClass *sourcePng, pngImageClass *targetPng, uint32 sourceX, uint32 sourceY, uint32 targetX, uint32 targetY, uint32 width, uint32 height);
}

#endif // guard_png_h_681DF37FA76B4FA48C656E96AF90EE69
