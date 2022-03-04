#include "image.h"

#include <cage-core/imageBlocks.h>
#include <cage-core/serialization.h>

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

	Holder<Image> imageAstcDecode(PointerRange<const char> buffer, const ImageAstcDecodeConfig &config_)
	{
		if (config_.resolution[0] <= 0 || config_.resolution[1] <= 0)
			CAGE_THROW_ERROR(Exception, "invalid resolution for astc decoding");

		astcenc_swizzle swizzle = {};
		switch (config_.channels)
		{
		case 1: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_0, ASTCENC_SWZ_0, ASTCENC_SWZ_0 }; break;
		case 2: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_A, ASTCENC_SWZ_0, ASTCENC_SWZ_0 }; break;
		case 3: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_0 }; break;
		case 4: swizzle = { ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A }; break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported channels count for astc decoding");
		}

		astcenc_image image = {};
		image.data_type = config_.format == ImageFormatEnum::U8 ? ASTCENC_TYPE_U8 : ASTCENC_TYPE_F32;
		image.dim_x = config_.resolution[0];
		image.dim_y = config_.resolution[1];
		image.dim_z = 1;

		astcenc_config config = {};
		config.block_x = config_.tiling[0];
		config.block_y = config_.tiling[1];
		config.block_z = 1;

		throwOnError(astcenc_config_init(config.profile, config.block_x, config.block_y, config.block_z, 0, config.flags | ASTCENC_FLG_DECOMPRESS_ONLY, &config), "astcenc_config_init");
		Context context(config);

		Holder<Image> img = newImage();
		img->initialize(config_.resolution, 4, config_.format == ImageFormatEnum::U8 ? ImageFormatEnum::U8 : ImageFormatEnum::Float);
		ImageImpl *impl = (ImageImpl *)+img;

		void *data = (void *)impl->mem.data();
		image.data = &data;
		throwOnError(astcenc_decompress_image(context.context, (const std::uint8_t *)buffer.data(), buffer.size(), &image, &swizzle, 0), "astcenc_decompress_image");

		imageConvert(+img, config_.channels);
		imageConvert(+img, config_.format);

		img->colorConfig = defaultConfig(config_.channels);

		return img;
	}

	Holder<PointerRange<char>> imageAstcEncode(const Image *image_, const ImageAstcEncodeConfig &config_)
	{
		const ImageImpl *impl = (const ImageImpl *)image_;

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
		void *data = (void *)impl->mem.data();
		image.data = &data;

		astcenc_config config = {};
		config.block_x = config_.tiling[0];
		config.block_y = config_.tiling[1];
		config.block_z = 1;

		switch (image_->format())
		{
		case ImageFormatEnum::Float:
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

		if (config_.normals)
			config.flags |= ASTCENC_FLG_MAP_NORMAL;
		else if (image_->colorConfig.alphaMode == AlphaModeEnum::None)
			config.flags |= ASTCENC_FLG_MAP_MASK;

		throwOnError(astcenc_config_init(config.profile, config.block_x, config.block_y, config.block_z, config_.quality, config.flags, &config), "astcenc_config_init");
		Context context(config);

		const uint32 blocksX = (image.dim_x + config.block_x - 1) / config.block_x;
		const uint32 blocksY = (image.dim_y + config.block_y - 1) / config.block_y;
		MemoryBuffer buff(blocksX * blocksY * 16);
		throwOnError(astcenc_compress_image(context.context, &image, &swizzle, (std::uint8_t *)buff.data(), buff.size(), 0), "astcenc_compress_image");

		return std::move(buff);
	}

	void astcDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		Deserializer des(inBuffer);
		astc_header header;
		des >> header;

		if ((header.dim_z[0] + (header.dim_z[1] << 8) + (header.dim_z[2] << 16)) != 1 || header.block_z != 1)
			CAGE_THROW_ERROR(Exception, "non-2D astc files are not supported");

		ImageAstcDecodeConfig config;
		config.resolution[0] = header.dim_x[0] + (header.dim_x[1] << 8) + (header.dim_x[2] << 16);
		config.resolution[1] = header.dim_y[0] + (header.dim_y[1] << 8) + (header.dim_y[2] << 16);
		config.tiling[0] = header.block_x;
		config.tiling[1] = header.block_y;

		Holder<Image> tmp = imageAstcDecode(des.read(des.available()), config);
		swapAll((ImageImpl *)+tmp, impl);
	}

	MemoryBuffer astcEncode(const ImageImpl *impl)
	{
		MemoryBuffer buff;
		Serializer ser(buff);

		ImageAstcEncodeConfig config;
		config.tiling = Vec2i(4, 4);
#ifdef CAGE_DEBUG
		config.quality = 10;
#endif // CAGE_DEBUG

		{
			astc_header header = {};
			header.magic[0] = 0x13;
			header.magic[1] = 0xAB;
			header.magic[2] = 0xA1;
			header.magic[3] = 0x5C;
			header.block_x = config.tiling[0];
			header.block_y = config.tiling[1];
			header.block_z = 1;
			header.dim_x[0] = impl->width >> 0;
			header.dim_x[1] = impl->width >> 8;
			header.dim_x[2] = impl->width >> 16;
			header.dim_y[0] = impl->height >> 0;
			header.dim_y[1] = impl->height >> 8;
			header.dim_y[2] = impl->height >> 16;
			header.dim_z[0] = 1;
			ser << header;
		}

		Holder<PointerRange<char>> tmp = imageAstcEncode(impl, config);
		ser.write(tmp);
		return buff;
	}
}
