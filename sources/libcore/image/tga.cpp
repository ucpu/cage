#include "image.h"
#include <cage-core/serialization.h>
#include <utility>

namespace cage
{
	namespace
	{
		template<typename Type>
		void rgbPalette(Deserializer &inBuffer, const uint8 *colorMap, Serializer &outBuffer, uint32 size)
		{
			for (uint32 i = 0; i < size; i++)
			{
				Type index;
				inBuffer >> index;
				const uint8 *cm = &colorMap[index * 3];
				uint8 red = cm[0], green = cm[1], blue = cm[2];
				outBuffer << blue << green << red;
			}
		}

		template<typename Type>
		void rgbaPalette(Deserializer &inBuffer, const uint8 *colorMap, Serializer &outBuffer, uint32 size)
		{
			for (uint32 i = 0; i < size; i++)
			{
				Type index;
				inBuffer >> index;
				const uint8 *cm = &colorMap[index * 4];
				uint8 red = cm[0], green = cm[1], blue = cm[2], alpha = cm[3];
				outBuffer << blue << green << red << alpha;
			}
		}

		void monoCompressed(Deserializer &inBuffer, Serializer &outBuffer, uint32 size)
		{
			for (uint32 i = 0; i < size; )
			{
				uint8 header;
				inBuffer >> header;
				uint32 pixelCount = (header & 0x7F) + 1;
				if (header & 0x80)
				{
					uint8 red;
					inBuffer >> red;
					for (uint32 j = 0; j < pixelCount; j++)
						outBuffer << red;
					i += pixelCount;
				}
				else
				{
					for (uint32 j = 0; j < pixelCount; j++)
					{
						uint8 red;
						inBuffer >> red;
						outBuffer << red;
					}
					i += pixelCount;
				}
			}
		}

		void rgbCompressed(Deserializer &inBuffer, Serializer &outBuffer, uint32 size)
		{
			for (uint32 i = 0; i < size; )
			{
				uint8 header;
				inBuffer >> header;
				uint32 pixelCount = (header & 0x7F) + 1;
				if (header & 0x80)
				{
					uint8 red, green, blue;
					inBuffer >> red >> green >> blue;
					for (uint32 j = 0; j < pixelCount; j++)
						outBuffer << blue << green << red;
					i += pixelCount;
				}
				else
				{
					for (uint32 j = 0; j < pixelCount; j++)
					{
						uint8 red, green, blue;
						inBuffer >> red >> green >> blue;
						outBuffer << blue << green << red;
					}
					i += pixelCount;
				}
			}
		}

		void rgbaCompressed(Deserializer &inBuffer, Serializer &outBuffer, uint32 size)
		{
			for (uint32 i = 0; i < size; )
			{
				uint8 header;
				inBuffer >> header;
				uint32 pixelCount = (header & 0x7F) + 1;
				if (header & 0x80)
				{
					uint8 red, green, blue, alpha;
					inBuffer >> red >> green >> blue >> alpha;
					for (uint32 j = 0; j < pixelCount; j++)
						outBuffer << blue << green << red << alpha;
					i += pixelCount;
				}
				else
				{
					for (uint32 j = 0; j < pixelCount; j++)
					{
						uint8 red, green, blue, alpha;
						inBuffer >> red >> green >> blue >> alpha;
						outBuffer << blue << green << red << alpha;
					}
					i += pixelCount;
				}
			}
		}

		void swapRB(uint8 *buffer)
		{
			std::swap(buffer[0], buffer[2]);
		}

		void swapRB(uint8 *buffer, uint32 samples, uint32 pixelSize)
		{
			for (uint32 i = 0; i < samples; i++)
				swapRB(buffer + i * pixelSize);
		}

		struct Header
		{
			uint8 idLength;
			uint8 colorMapType;
			uint8 imageType;
			uint16 colorMapOrigin;
			uint16 colorMapLength;
			uint8 colorMapEntrySize;
			uint16 xOrigin;
			uint16 yOrigin;
			uint16 width;
			uint16 height;
			uint8 bits;
			uint8 imageDescriptor;
		};

		void tgaDecodeImpl(PointerRange<const char> inBuffer, ImageImpl *impl)
		{
			Deserializer des(inBuffer);

			Header head;
			des >> head.idLength;
			des >> head.colorMapType;
			des >> head.imageType;
			des >> head.colorMapOrigin;
			des >> head.colorMapLength;
			des >> head.colorMapEntrySize;
			des >> head.xOrigin;
			des >> head.yOrigin;
			des >> head.width;
			des >> head.height;
			des >> head.bits;
			des >> head.imageDescriptor;

			// skip
			des.advance(head.imageDescriptor);

			const uint32 colorMapElementSize = head.colorMapEntrySize / 8;
			const uint32 pixelSize = head.colorMapLength == 0 ? (head.bits / 8) : colorMapElementSize;
			const uint32 pixelsCount = (uint32)head.width * (uint32)head.height;
			const uint32 imageSize = pixelsCount * pixelSize;
			const uint32 colorMapSize = head.colorMapLength * colorMapElementSize;
			const uint8 *colorMap = nullptr;
			if (head.colorMapType == 1)
				colorMap = (uint8*)des.advance(colorMapSize);

			impl->width = head.width;
			impl->height = head.height;
			impl->channels = pixelSize;
			impl->format = ImageFormatEnum::U8;
			impl->mem.resize(0);
			impl->mem.reserve(imageSize);

			Serializer ser(impl->mem);
			switch (head.imageType)
			{
			case 1: // uncompressed palette
			{
				if (head.bits == 8)
				{
					switch (pixelSize)
					{
					case 3: return rgbPalette<uint8>(des, colorMap, ser, pixelsCount);
					case 4: return rgbaPalette<uint8>(des, colorMap, ser, pixelsCount);
					}
				}
				else if (head.bits == 16)
				{
					switch (pixelSize)
					{
					case 3: return rgbPalette<uint16>(des, colorMap, ser, pixelsCount);
					case 4: return rgbaPalette<uint16>(des, colorMap, ser, pixelsCount);
					}
				}
			} break;
			case 2: // uncompressed color
			{
				if (head.bits == 24 || head.bits == 32)
				{
					ser.write(des.advance(imageSize), imageSize);
					swapRB((uint8*)impl->mem.data(), pixelsCount, pixelSize);
					return;
				}
			} break;
			case 3: // uncompressed mono
			{
				return ser.write(des.advance(imageSize), imageSize);
			} break;
			case 10: // compressed color
			{
				switch (head.bits)
				{
				case 24: return rgbCompressed(des, ser, pixelsCount);
				case 32: return rgbaCompressed(des, ser, pixelsCount);
				}
			} break;
			case 11: // compressed mono
			{
				if (head.bits == 8)
					return monoCompressed(des, ser, pixelsCount);
			} break;
			}

			CAGE_THROW_ERROR(Exception, "unsupported format in tga decoding");
		}
	}

	void tgaDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		tgaDecodeImpl(inBuffer, impl);
		impl->verticalFlip();
		impl->colorConfig = defaultConfig(impl->channels);
	}

	MemoryBuffer tgaEncode(ImageImpl *impl)
	{
		if (impl->format != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for tga encoding");
		if (impl->width >= 65536 || impl->height >= 65536)
			CAGE_THROW_ERROR(Exception, "unsupported image resolution for tga encoding");

		MemoryBuffer buf;
		buf.reserve(impl->width * impl->height * impl->channels + 100);
		Serializer ser(buf);

		Header head;
		detail::memset(&head, 0, sizeof(head));
		switch (impl->channels)
		{
		case 1:
			head.imageType = 3; // uncompressed mono
			head.bits = 8;
			break;
		case 3:
			head.imageType = 2; // uncompressed color
			head.bits = 24;
			break;
		case 4:
			head.imageType = 2; // uncompressed color
			head.bits = 32;
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported image channels for tga encoding");
		}
		head.width = impl->width;
		head.height = impl->height;

		ser << head.idLength;
		ser << head.colorMapType;
		ser << head.imageType;
		ser << head.colorMapOrigin;
		ser << head.colorMapLength;
		ser << head.colorMapEntrySize;
		ser << head.xOrigin;
		ser << head.yOrigin;
		ser << head.width;
		ser << head.height;
		ser << head.bits;
		ser << head.imageDescriptor;

		const uint32 scanline = impl->width * impl->channels;
		const uint32 size = scanline * impl->height;
		uint8 *pixels = (uint8*)ser.advance(size);
		for (uint32 i = 0; i < impl->height; i++)
			detail::memcpy(pixels + (impl->height - i - 1) * scanline, impl->mem.data() + i * scanline, scanline);
		if (impl->channels > 1)
			swapRB(pixels, impl->width * impl->height, impl->channels);

		const uint32 dummy = 0;
		ser << dummy << dummy;
		ser.write("TRUEVISION-XFILE.", 18);

		return buf;
	}
}
