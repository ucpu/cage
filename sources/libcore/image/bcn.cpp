#include "image.h"

#include <cage-core/imageBlocks.h>
#include <cage-core/pointerRangeHolder.h>
#include <vector>

#include <bc7enc_rdo/bc7decomp.h>
#include <bc7enc_rdo/bc7enc.h>
#include <bc7enc_rdo/rgbcx.h>

namespace cage
{
	namespace
	{
		void initRgbcx()
		{
			static const int initDummy = []()
			{
				rgbcx::init();
				return 0;
			}();
		}

		union Color
		{
			uint8 rgba[4];
			uint32 data;
			Color() : rgba{ 0, 0, 0, 255 } {}
		};
		static_assert(sizeof(Color) == 4);

		CAGE_FORCE_INLINE uint32 blockyIndex(uint32 x, uint32 y, uint32 width)
		{
			const uint32 bx = x / 4;
			const uint32 by = y / 4;
			const uint32 bw = width / 4;
			const uint32 block = by * bw + bx;
			const uint32 pixelInBlock = (y % 4) * 4 + (x % 4);
			return block * 16 + pixelInBlock;
		}

		template<uint32 Channels, uint32 BytesPerBlock, class Function>
		Holder<PointerRange<char>> encoder(const Image *img, const ImageBcnEncodeConfig &config, const Function &function)
		{
			CAGE_ASSERT(img->channels() == Channels);
			const Vec2i resolution_ = img->resolution();
			const Vec2i resolution = 4 * ((resolution_ + 3) / 4); // round up to multiple of 4x4
			const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
			std::vector<Color> colors; // todo rewrite to streaming version avoiding this vector
			colors.resize(blocksRequired * 16);
			for (uint32 y = 0; y < resolution_[1]; y++)
			{
				for (uint32 x = 0; x < resolution_[0]; x++)
				{
					Color &color = colors[blockyIndex(x, y, resolution[0])];
					for (uint32 c = 0; c < Channels; c++)
						color.rgba[c] = numeric_cast<uint8>(img->value(x, y, c) * 255);
				}
			}
			PointerRangeHolder<char> buffer;
			buffer.resize(blocksRequired * BytesPerBlock);
			for (uint32 bi = 0; bi < blocksRequired; bi++)
				function(buffer.data() + bi * BytesPerBlock, (colors.data() + bi * 16)->rgba);
			return buffer;
		}

		template<uint32 Channels, uint32 BytesPerBlock, class Function>
		Holder<Image> decoder(PointerRange<const char> buffer, const Vec2i &resolution_, const Function &function)
		{
			const Vec2i resolution = 4 * ((resolution_ + 3) / 4); // round up to multiple of 4x4
			const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
			if (blocksRequired * BytesPerBlock != buffer.size())
				CAGE_THROW_ERROR(Exception, "incorrect data size for bcn decoding");
			std::vector<Color> colors; // todo rewrite to streaming version avoiding this vector
			colors.resize(blocksRequired * 16);
			for (uint32 bi = 0; bi < blocksRequired; bi++)
				function((colors.data() + bi * 16)->rgba, buffer.data() + bi * BytesPerBlock);
			Holder<Image> img = newImage();
			img->initialize(resolution_, Channels, ImageFormatEnum::U8);
			for (uint32 y = 0; y < resolution_[1]; y++)
			{
				for (uint32 x = 0; x < resolution_[0]; x++)
				{
					const Color color = colors[blockyIndex(x, y, resolution[0])];
					for (uint32 c = 0; c < Channels; c++)
						img->value(x, y, c, color.rgba[c] / 255.f);
				}
			}
			return img;
		}
	}

	Holder<PointerRange<char>> imageBc1Encode(const Image *image, const ImageBcnEncodeConfig &config)
	{
		if (image->channels() != 3)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc1 encoding");
		initRgbcx();
		struct Fnc
		{
			void operator()(void *dst, const uint8 *src) const { rgbcx::encode_bc1(rgbcx::MAX_LEVEL, dst, src, true, false); }
		};
		return encoder<3, 8>(image, config, Fnc());
	}

	Holder<PointerRange<char>> imageBc3Encode(const Image *image, const ImageBcnEncodeConfig &config)
	{
		if (image->channels() != 4)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc3 encoding");
		initRgbcx();
		struct Fnc
		{
			void operator()(void *dst, const uint8 *src) const { rgbcx::encode_bc3(rgbcx::MAX_LEVEL, dst, src); }
		};
		return encoder<4, 16>(image, config, Fnc());
	}

	Holder<PointerRange<char>> imageBc4Encode(const Image *image, const ImageBcnEncodeConfig &config)
	{
		if (image->channels() != 1)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc4 encoding");
		initRgbcx();
		struct Fnc
		{
			void operator()(void *dst, const uint8 *src) const { rgbcx::encode_bc4(dst, src); }
		};
		return encoder<1, 8>(image, config, Fnc());
	}

	Holder<PointerRange<char>> imageBc5Encode(const Image *image, const ImageBcnEncodeConfig &config)
	{
		if (image->channels() != 2)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc5 encoding");
		initRgbcx();
		struct Fnc
		{
			void operator()(void *dst, const uint8 *src) const { rgbcx::encode_bc5(dst, src); }
		};
		return encoder<2, 16>(image, config, Fnc());
	}

	Holder<PointerRange<char>> imageBc7Encode(const Image *image, const ImageBcnEncodeConfig &config)
	{
		if (image->channels() != 3 && image->channels() != 4)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc7 encoding");
		static const int initDummy = []()
		{
			bc7enc_compress_block_init();
			return 0;
		}();
		struct Fnc
		{
			bc7enc_compress_block_params conf;
			void operator()(void *dst, const uint8 *src) const { bc7enc_compress_block(dst, src, &conf); }
		};
		Fnc fnc;
		bc7enc_compress_block_params_init(&fnc.conf);
		if (image->channels() == 3)
			return encoder<3, 16>(image, config, fnc);
		if (image->channels() == 4)
			return encoder<4, 16>(image, config, fnc);
		throw;
	}

	Holder<Image> imageBc1Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		struct Fnc
		{
			void operator()(uint8 *dst, const void *src) const { rgbcx::unpack_bc1(src, dst); }
		};
		return decoder<3, 8>(buffer, resolution, Fnc());
	}

	namespace bc2
	{
		struct Bc2Block // r5g6b5a4 dxt3 uncorrelated alpha
		{
			uint16 alphaRow[4];
			uint16 color0;
			uint16 color1;
			uint8 row[4];
		};
		static_assert(sizeof(Bc2Block) == 16);

		struct U8Block
		{
			uint8 texel[4][4][4];
		};

		struct F32Block
		{
			Vec4 texel[4][4];
		};

		CAGE_FORCE_INLINE Vec3 unpack565(uint16 v)
		{
			const uint8 x = (v >> 0) & 31u;
			const uint8 y = (v >> 5) & 63u;
			const uint8 z = (v >> 11) & 31u;
			return Vec3(x, y, z) * Vec3(1.0 / 31, 1.0 / 63, 1.0 / 31);
		}

		CAGE_FORCE_INLINE U8Block F32ToU8Block(const F32Block &in)
		{
			U8Block res;
			for (uint32 y = 0; y < 4; y++)
				for (uint32 x = 0; x < 4; x++)
					for (uint32 c = 0; c < 4; c++)
						res.texel[y][x][c] = numeric_cast<uint8>(in.texel[y][x][c] * 255);
			return res;
		}

		void bcDecompress(const Bc2Block &in, U8Block &out)
		{
			Vec3 color[4];
			color[0] = Vec3(unpack565(in.color0));
			std::swap(color[0][0], color[0][2]);
			color[1] = Vec3(unpack565(in.color1));
			std::swap(color[1][0], color[1][2]);
			color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
			color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];

			F32Block texelBlock;
			for (uint8 row = 0; row < 4; row++)
			{
				for (uint8 col = 0; col < 4; col++)
				{
					uint8 colorIndex = (in.row[row] >> (col * 2)) & 0x3;
					Real alpha = ((in.alphaRow[row] >> (col * 4)) & 0xF) / 15.0f;
					texelBlock.texel[row][col] = Vec4(color[colorIndex], alpha);
				}
			}

			out = F32ToU8Block(texelBlock);
		}
	}

	Holder<Image> imageBc2Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		struct Fnc
		{
			void operator()(uint8 *dst, const void *src) const { bc2::bcDecompress(*(const bc2::Bc2Block *)src, *(bc2::U8Block *)dst); }
		};
		return decoder<4, 16>(buffer, resolution, Fnc());
	}

	Holder<Image> imageBc3Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		struct Fnc
		{
			void operator()(uint8 *dst, const void *src) const { rgbcx::unpack_bc3(src, dst); }
		};
		return decoder<4, 16>(buffer, resolution, Fnc());
	}

	Holder<Image> imageBc4Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		struct Fnc
		{
			void operator()(uint8 *dst, const void *src) const { rgbcx::unpack_bc4(src, dst); }
		};
		return decoder<1, 8>(buffer, resolution, Fnc());
	}

	Holder<Image> imageBc5Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		struct Fnc
		{
			void operator()(uint8 *dst, const void *src) const { rgbcx::unpack_bc5(src, dst); }
		};
		return decoder<2, 16>(buffer, resolution, Fnc());
	}

	Holder<Image> imageBc7Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		struct Fnc
		{
			void operator()(uint8 *dst, const void *src) const { bc7decomp::unpack_bc7(src, (bc7decomp::color_rgba *)dst); }
		};
		return decoder<4, 16>(buffer, resolution, Fnc());
	}
}
