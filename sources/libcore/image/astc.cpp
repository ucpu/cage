#include "image.h"

#include <astcenc.h>

namespace cage
{
	namespace
	{
		void throwOnError(astcenc_error error, StringLiteral msg)
		{
			if (error != ASTCENC_SUCCESS)
			{
				CAGE_LOG_THROW(astcenc_get_error_string(error));
				CAGE_THROW_ERROR(Exception, msg);
			}
		}

		struct Context : private Immovable
		{
			astcenc_context *context = nullptr;

			Context(const astcenc_config &config)
			{
				throwOnError(astcenc_context_alloc(&config, 1, &context), "astcenc_context_alloc");
				CAGE_ASSERT(context);
			}

			~Context()
			{
				if (context)
					astcenc_context_free(context);
			}
		};

		// https://github.com/ARM-software/astc-encoder/blob/a287627df60809a2a32fe959757d7b3112716a6f/Docs/FileFormat.md
		struct astc_header
		{
			uint8_t magic[4];
			uint8_t block_x;
			uint8_t block_y;
			uint8_t block_z;
			uint8_t dim_x[3];
			uint8_t dim_y[3];
			uint8_t dim_z[3];
		};
	}

	void astcDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		if (inBuffer.size() <= sizeof(astc_header))
			CAGE_THROW_ERROR(Exception, "astc file has insufficient size");

		astc_header &header = *(astc_header *)inBuffer.data();

		const astcenc_swizzle swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

		astcenc_image image = {};
		image.data_type = ASTCENC_TYPE_U8;
		image.dim_x = header.dim_x[0] + (header.dim_x[1] << 8) + (header.dim_x[2] << 16);
		image.dim_y = header.dim_y[0] + (header.dim_y[1] << 8) + (header.dim_y[2] << 16);
		image.dim_z = 1;

		astcenc_config config = {};
		config.block_x = header.block_x;
		config.block_y = header.block_y;
		config.block_z = header.block_z;
		config.profile = ASTCENC_PRF_LDR;

		throwOnError(astcenc_config_init(config.profile, config.block_x, config.block_y, config.block_z, 0, config.flags | ASTCENC_FLG_DECOMPRESS_ONLY, &config), "astcenc_config_init");
		Context context(config);

		impl->channels = 4;
		impl->width = image.dim_x;
		impl->height = image.dim_y;
		impl->format = ImageFormatEnum::U8;
		impl->colorConfig = defaultConfig(impl->channels);

		impl->mem.resize(impl->width * impl->height * 4);
		void *data = (void *)impl->mem.data();
		image.data = &data;
		throwOnError(astcenc_decompress_image(context.context, (const std::uint8_t *)inBuffer.data(), inBuffer.size(), &image, &swizzle, 0), "astcenc_decompress_image");
	}

	MemoryBuffer astcEncode(const ImageImpl *impl)
	{
		astcenc_swizzle swizzle = {};
		switch (impl->channels)
		{
		case 1: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_1 }; break;
		case 2: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_G }; break;
		case 3: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_1 }; break;
		case 4: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A }; break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported channels count for astc encoding");
		}

		Holder<Image> implHld;
		if (impl->channels < 4)
		{
			implHld = impl->copy();
			impl = (ImageImpl *)+implHld;
			if (impl->format == ImageFormatEnum::Rgbe)
				imageConvert(+implHld, ImageFormatEnum::Float);
			imageConvert(+implHld, 4);
		}

		astcenc_image image = {};
		image.dim_x = impl->width;
		image.dim_y = impl->height;
		image.dim_z = 1;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
			image.data_type = ASTCENC_TYPE_U8;
			break;
		case ImageFormatEnum::Float:
			image.data_type = ASTCENC_TYPE_F32;
			break;
		default:
		{
			if (!implHld)
			{
				implHld = impl->copy();
				impl = (ImageImpl *)+implHld;
			}
			imageConvert(+implHld, ImageFormatEnum::Float);
			image.data_type = ASTCENC_TYPE_F32;
		} break;
		}
		void *data = (void*)impl->mem.data();
		image.data = &data;

		float quality = ASTCENC_PRE_THOROUGH;
#ifdef CAGE_DEBUG
		quality = ASTCENC_PRE_FAST;
#endif // CAGE_DEBUG

		astcenc_config config = {};
		config.block_x = 6;
		config.block_y = 5;
		config.block_z = 1;

		switch (impl->format)
		{
		case ImageFormatEnum::Float:
		case ImageFormatEnum::Rgbe:
		{
			if (impl->colorConfig.alphaMode == AlphaModeEnum::Opacity)
				config.profile = ASTCENC_PRF_HDR_RGB_LDR_A;
			else
				config.profile = ASTCENC_PRF_HDR;
		} break;
		default:
		{
			if (impl->colorConfig.gammaSpace == GammaSpaceEnum::Gamma)
				config.profile = ASTCENC_PRF_LDR_SRGB;
			else
				config.profile = ASTCENC_PRF_LDR;
		} break;
		}

		throwOnError(astcenc_config_init(config.profile, config.block_x, config.block_y, config.block_z, quality, config.flags, &config), "astcenc_config_init");
		Context context(config);

		const uint32 blocksX = (image.dim_x + config.block_x - 1) / config.block_x;
		const uint32 blocksY = (image.dim_y + config.block_y - 1) / config.block_y;
		const uint32 compSize = blocksX * blocksY * 16;
		MemoryBuffer buff(sizeof(astc_header) + compSize);
		throwOnError(astcenc_compress_image(context.context, &image, &swizzle, (std::uint8_t*)buff.data() + sizeof(astc_header), compSize, 0), "astcenc_compress_image");

		astc_header &header = *(astc_header *)buff.data();
		header = {};
		header.magic[0] = 0x13;
		header.magic[1] = 0xAB;
		header.magic[2] = 0xA1;
		header.magic[3] = 0x5C;
		header.block_x = config.block_x;
		header.block_y = config.block_y;
		header.block_z = config.block_z;
		header.dim_x[0] = image.dim_x >> 0;
		header.dim_x[1] = image.dim_x >> 8;
		header.dim_x[2] = image.dim_x >> 16;
		header.dim_y[0] = image.dim_y >> 0;
		header.dim_y[1] = image.dim_y >> 8;
		header.dim_y[2] = image.dim_y >> 16;
		return buff;
	}
}
