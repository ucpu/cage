#include "image.h"
#include <cage-core/serialization.h>
#include <cage-core/endianness.h>
#include <utility>

namespace cage
{
	namespace
	{
		void decodeRaw(Deserializer &des, ImageImpl *impl)
		{
			switch (impl->format)
			{
			case ImageFormatEnum::U8:
			{
				uint8 *dst = (uint8*)impl->mem.data();
				for (uint32 ch = 0; ch < impl->channels; ch++)
					for (uint32 y = 0; y < impl->height; y++)
						for (uint32 x = 0; x < impl->width; x++)
							des >> dst[(y * impl->width + x) * impl->channels + ch];
			} break;
			case ImageFormatEnum::U16:
			{
				uint16 *dst = (uint16*)impl->mem.data();
				for (uint32 ch = 0; ch < impl->channels; ch++)
				{
					for (uint32 y = 0; y < impl->height; y++)
					{
						for (uint32 x = 0; x < impl->width; x++)
						{
							uint16 &d = dst[(y * impl->width + x) * impl->channels + ch];
							des >> d;
							d = endianness::change(d);
						}
					}
				}
			} break;
			default:
				CAGE_THROW_ERROR(Exception, "unsupported image format in psd decoding");
			}
		}

		void decodeRLE(Deserializer &des, ImageImpl *impl)
		{
			MemoryBuffer buf;
			buf.reserve(impl->mem.size());
			Serializer ser(buf);

			uint32 scanlines = impl->height * impl->channels;
			uint32 linewidth = impl->width * formatBytes(impl->format);
			des.advance(scanlines * 2);

			for (uint32 line = 0; line < scanlines; line++)
			{
				Serializer ser2 = ser.placeholder(linewidth);
				while (ser2.available())
				{
					sint32 rl;
					{
						sint8 b;
						des >> b;
						rl = b;
					}
					if (rl <= 0)
					{
						// copy one byte many times
						rl = rl * -1 + 1;
						uint8 v;
						des >> v;
						detail::memset(ser2.advance(rl).data(), v, rl);
					}
					else
					{
						// copy many bytes as is
						rl += 1;
						ser2.write(des.advance(rl));
					}
				}
			}

			Deserializer des2(buf);
			decodeRaw(des2, impl);
		}

		void decodeZip(Deserializer &des, ImageImpl *impl, bool prediction)
		{
			CAGE_THROW_ERROR(NotImplemented, "unsupported compression method (zip) in psd decoding");
		}

		void encodeRaw(Serializer &ser, const ImageImpl *impl)
		{
			switch (impl->format)
			{
			case ImageFormatEnum::U8:
			{
				const uint8 *src = (uint8*)impl->mem.data();
				for (uint32 ch = 0; ch < impl->channels; ch++)
					for (uint32 y = 0; y < impl->height; y++)
						for (uint32 x = 0; x < impl->width; x++)
							ser << src[(y * impl->width + x) * impl->channels + ch];
			} break;
			case ImageFormatEnum::U16:
			{
				const uint16 *src = (uint16*)impl->mem.data();
				for (uint32 ch = 0; ch < impl->channels; ch++)
					for (uint32 y = 0; y < impl->height; y++)
						for (uint32 x = 0; x < impl->width; x++)
							ser << endianness::change<uint16>(src[(y * impl->width + x) * impl->channels + ch]);
			} break;
			default:
				CAGE_THROW_ERROR(Exception, "unsupported image format for psd encoding");
			}
		}
	}

	void psdDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		Deserializer des(inBuffer);

		// file header section

		// signature
		des.advance(4);

		// version
		uint16 version;
		des >> version;
		version = endianness::change(version);
		switch (version)
		{
		case 1:
		case 2:
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported version in psd decoding");
		}

		// reserved
		des.advance(6);

		// channels
		{
			uint16 channels;
			des >> channels;
			channels = endianness::change(channels);
			impl->channels = channels;
		}

		// resolution
		des >> impl->height >> impl->width;
		impl->width = endianness::change(impl->width);
		impl->height = endianness::change(impl->height);

		// depth
		{
			uint16 depth;
			des >> depth;
			depth = endianness::change(depth);
			switch (depth)
			{
			case 8:
				impl->format = ImageFormatEnum::U8;
				break;
			case 16:
				impl->format = ImageFormatEnum::U16;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "unsupported depth in psd decoding");
			}
		}

		// color mode
		des.advance(2);

		// color mode data section
		{
			uint32 len;
			des >> len;
			len = endianness::change(len);
			des.advance(len); // ignore
		}

		// image resources section
		{
			uint32 len;
			des >> len;
			len = endianness::change(len);
			des.advance(len); // ignore
		}

		// layer and mask information section
		switch (version)
		{
		case 1:
		{
			uint32 len;
			des >> len;
			len = endianness::change(len);
			des.advance(len); // ignore
		} break;
		case 2:
		{
			uint64 len;
			des >> len;
			len = endianness::change(len);
			des.advance(len); // ignore
		} break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported version in psd decoding");
		}

		// image data section
		impl->mem.allocate(impl->width * impl->height * impl->channels * formatBytes(impl->format));

		// compression method
		uint16 compression;
		des >> compression;
		compression = endianness::change(compression);
		switch (compression)
		{
		case 0:
			decodeRaw(des, impl);
			break;
		case 1:
			decodeRLE(des, impl);
			break;
		case 2:
			decodeZip(des, impl, false);
			break;
		case 3:
			decodeZip(des, impl, true);
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported compression method in psd decoding");
		}

		// color config
		// todo deduce it from the file
	}

	MemoryBuffer psdEncode(const ImageImpl *impl)
	{
		if (impl->channels > 56)
			CAGE_THROW_ERROR(Exception, "unsupported image channels count for psd encoding");
		if (impl->width > 300000 || impl->height > 300000)
			CAGE_THROW_ERROR(Exception, "unsupported image resolution for psd encoding");

		MemoryBuffer buf;
		buf.reserve(impl->width * impl->height * impl->channels * formatBytes(impl->format) + 100);
		Serializer ser(buf);

		// file header section
		// signature
		ser << '8' << 'B' << 'P' << 'S';
		// version
		ser << endianness::change<uint16>(2);
		// reserved
		ser << uint16(0) << uint16(0) << uint16(0);
		// channels
		ser << endianness::change<uint16>(impl->channels);
		// resolution
		ser << endianness::change<uint32>(impl->height);
		ser << endianness::change<uint32>(impl->width);
		// depth
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
			ser << endianness::change<uint16>(8);
			break;
		case ImageFormatEnum::U16:
			ser << endianness::change<uint16>(16);
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported image format for psd encoding");
		}
		// color mode
		switch (impl->channels)
		{
		case 1:
			ser << endianness::change<uint16>(1); // grayscale
			break;
		case 3:
		case 4:
			ser << endianness::change<uint16>(3); // rgb
			break;
		default:
			ser << endianness::change<uint16>(7); // multichannel
			break;
		}

		// color mode data section
		ser << uint32(0); // zero length

		// image resources section
		ser << uint32(0); // zero length

		// layer and mask information section
		ser << uint64(0); // zero length

		// image data section
		// compression method
		ser << endianness::change<uint16>(0); // raw image data
		// pixels
		encodeRaw(ser, impl);

		return buf;
	}
}
