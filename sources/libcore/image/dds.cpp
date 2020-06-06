#include "image.h"
#include "dxt.h"
#include <cage-core/serialization.h>

// mipmaps, cubemaps, arrays, volumes are not supported
// only DXT1, DXT3 and DXT5 formats are supported

// inspiration taken from:
// https://gist.github.com/tilkinsc/13191c0c1e5d6b25fbe79bbd2288a673

namespace cage
{
	namespace
	{
		const uint32 Magic = 0x20534444;

		template<class Block>
		void decompress(Deserializer &des, ImageImpl *impl)
		{
			Serializer masterSer(impl->mem);
			const uint32 cols = impl->width / 4; // blocks count
			const uint32 rows = impl->height / 4; // blocks count
			const uint32 lineSize = impl->width * 4; // bytes count
			for (uint32 y = 0; y < rows; y++)
			{
				Serializer tmpSer0 = masterSer.placeholder(lineSize);
				Serializer tmpSer1 = masterSer.placeholder(lineSize);
				Serializer tmpSer2 = masterSer.placeholder(lineSize);
				Serializer tmpSer3 = masterSer.placeholder(lineSize);
				Serializer *lineSer[4] = { &tmpSer0, &tmpSer1, &tmpSer2, &tmpSer3 };
				for (uint32 x = 0; x < cols; x++)
				{
					Block block;
					des >> block;
					TexelBlock texels = dxtDecompress(block);
					for (uint32 b = 0; b < 4; b++)
					{
						for (uint32 a = 0; a < 4; a++)
						{
							vec4 tex = texels.texel[b][a] * 255;
							for (uint32 i = 0; i < 4; i++)
								*(lineSer[b]) << numeric_cast<uint8>(tex[i]);
						}
					}
				}
			}
			CAGE_ASSERT(masterSer.available() == 0);
		}

		void compress(Serializer &ser, ImageImpl *impl)
		{
			const uint32 cols = impl->width / 4;
			const uint32 rows = impl->height / 4;
			const uint32 *imageBase = (const uint32 *)impl->mem.data();
			for (uint32 y = 0; y < rows; y++)
			{
				const uint32 *lineBase = imageBase + y * 4 * impl->width;
				for (uint32 x = 0; x < cols; x++)
				{
					const uint32 *block = lineBase + x * 4;
					TexelBlock texels;
					for (uint32 b = 0; b < 4; b++)
					{
						const uint32 *line = block + b * impl->width;
						for (uint32 a = 0; a < 4; a++)
						{
							const uint8 *t = (const uint8 *)(line + a);
							texels.texel[b][a] = vec4(t[0], t[1], t[2], t[3]) / 255;
						}
					}
					ser << dxtCompress(texels);
				}
			}
		}

		struct PixelFormat
		{
			uint32 size;
			uint32 flags;
			uint32 fourCC;
			uint32 bpp;
			uint32 mask[4];
		};

		struct Header
		{
			uint32 magic;
			uint32 size;
			uint32 flags;
			uint32 height;
			uint32 width;
			uint32 pitch;
			uint32 depth;
			uint32 mipMapLevels;
			uint32 reserved1[11];
			PixelFormat format;
			uint32 surfaceFlags;
			uint32 cubemapFlags;
			uint32 reserved2[3];
		};

		static_assert(sizeof(PixelFormat) == 32, "dds pixel format struct size mismatch");
		static_assert(sizeof(Header) == 128, "dds header struct size mismatch");

		constexpr const uint32 makeFourCC(const char *value)
		{
			return value[0] + (value[1] << 8) + (value[2] << 16) + (value[3] << 24);
		}
	}

	void ddsDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		Deserializer des(inBuffer);

		Header header;
		des >> header;
		CAGE_ASSERT(header.magic == Magic);

		if ((header.width % 4) != 0 || (header.height % 4) != 0)
			CAGE_THROW_ERROR(Exception, "unsupported resolution in dds decoding");

		impl->width = header.width;
		impl->height = header.height;
		impl->channels = 4;
		impl->format = ImageFormatEnum::U8;
		impl->mem.allocate(impl->width * impl->height * impl->channels);

		switch (header.format.fourCC)
		{
		case makeFourCC("DXT1"):
		{
			decompress<Dxt1Block>(des, impl);
		} break;
		case makeFourCC("DXT3"):
		{
			decompress<Dxt3Block>(des, impl);
		} break;
		case makeFourCC("DXT5"):
		{
			decompress<Dxt5Block>(des, impl);
		} break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported DXT (image compression) format in dds decoding");
		}

		impl->colorConfig = defaultConfig(impl->channels);
	}

	MemoryBuffer ddsEncode(ImageImpl *impl)
	{
		if (impl->format != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for dds encoding");
		if (impl->channels != 3 && impl->channels != 4)
			CAGE_THROW_ERROR(Exception, "unsupported image channels count for dds encoding");
		if ((impl->width % 4) != 0 || (impl->height % 4) != 0)
			CAGE_THROW_ERROR(Exception, "unsupported image resolution for dds encoding");

		Header header;
		detail::memset(&header, 0, sizeof(Header));
		header.magic = Magic;
		header.size = 124;
		header.flags = 0x2 + 0x4 + 0x1000;
		header.height = impl->height;
		header.width = impl->width;
		header.format.size = 32;
		header.format.flags = 0x4;
		header.format.fourCC = makeFourCC("DXT5");

		MemoryBuffer buf;
		buf.reserve(impl->width * impl->height * impl->channels + 128);
		Serializer ser(buf);

		ser << header;
		compress(ser, impl);

		return buf;
	}
}
