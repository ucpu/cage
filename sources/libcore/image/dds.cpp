#include "image.h"

#include <cage-core/imageAlgorithms.h>
#include <cage-core/imageBlocks.h>
#include <cage-core/serialization.h>
#include <cage-core/imageImport.h>
#include <cage-core/pointerRangeHolder.h>

// todo
// mipmaps, cubemaps, arrays, volumes are not supported
// only DXT1, DXT3 and DXT5 formats are supported

// inspiration taken from:
// https://gist.github.com/tilkinsc/13191c0c1e5d6b25fbe79bbd2288a673

namespace cage
{
	namespace
	{
		constexpr uint32 Magic = 0x20534444;

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

		consteval const uint32 makeFourCC(const char *value)
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

		switch (header.format.fourCC)
		{
		case makeFourCC("DXT1"):
		{
			auto res = imageBc1Decode(des.read(header.width * header.height / 2), Vec2i(header.width, header.height));
			swapAll(impl, (ImageImpl *)+res);
		} break;
		case makeFourCC("DXT3"):
		{
			auto res = imageBc2Decode(des.read(header.width * header.height), Vec2i(header.width, header.height));
			swapAll(impl, (ImageImpl *)+res);
		} break;
		case makeFourCC("DXT5"):
		{
			auto res = imageBc3Decode(des.read(header.width * header.height), Vec2i(header.width, header.height));
			swapAll(impl, (ImageImpl *)+res);
		} break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported DXT (image compression) format in dds decoding");
		}

		impl->colorConfig = defaultConfig(impl->channels);
	}

	ImageImportResult ddsDecode(PointerRange<const char> inBuffer, const ImageImportConfig &config)
	{
		Deserializer des(inBuffer);

		Header header;
		des >> header;
		CAGE_ASSERT(header.magic == Magic);

		if ((header.width % 4) != 0 || (header.height % 4) != 0)
			CAGE_THROW_ERROR(Exception, "unsupported resolution in dds decoding");

		ImageImportRaw raw;
		raw.resolution = Vec2i(header.width, header.height);
		raw.channels = 4;

		uint32 bufferSize = 0;
		switch (header.format.fourCC)
		{
		case makeFourCC("DXT1"):
			bufferSize = header.width * header.height / 2;
			raw.format = "bc1";
			raw.channels = 3;
			break;
		case makeFourCC("DXT3"):
			bufferSize = header.width * header.height;
			raw.format = "bc2";
			raw.channels = 4;
			break;
		case makeFourCC("DXT5"):
			bufferSize = header.width * header.height;
			raw.format = "bc3";
			raw.channels = 4;
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported DXT (image compression) format in dds decoding");
		}
		CAGE_ASSERT(bufferSize > 0);

		{
			PointerRangeHolder<char> data;
			data.resize(bufferSize);
			detail::memcpy(data.data(), des.read(bufferSize).data(), bufferSize);
			raw.data = std::move(data);
		}

		ImageImportPart part;
		part.raw = systemMemory().createHolder<ImageImportRaw>(std::move(raw));
		part.raw->colorConfig = defaultConfig(raw.channels);
		PointerRangeHolder<ImageImportPart> parts;
		parts.push_back(std::move(part));
		ImageImportResult result;
		result.parts = std::move(parts);
		return result;
	}

	MemoryBuffer ddsEncode(const ImageImpl *impl)
	{
		if (impl->format != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for dds encoding");
		if (impl->channels != 3 && impl->channels != 4)
			CAGE_THROW_ERROR(Exception, "unsupported image channels count for dds encoding");
		if ((impl->width % 4) != 0 || (impl->height % 4) != 0)
			CAGE_THROW_ERROR(Exception, "unsupported image resolution for dds encoding");

		if (impl->channels == 3)
		{
			Holder<Image> img = impl->copy();
			imageConvert(+img, 4);
			imageFill(+img, 3, 1);
			img->colorConfig.alphaMode = AlphaModeEnum::None;
			return ddsEncode((ImageImpl *)+img);
		}

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
		Serializer ser(buf);
		ser << header;
		ser.write(imageBc3Encode(impl));
		return buf;
	}
}
