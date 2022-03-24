#include "image.h"
#include <cage-core/imageBlocks.h>
#include <cage-core/serialization.h>

#include <basis_universal/encoder/basisu_gpu_texture.h>

#include <vector>

namespace cage
{
	Holder<PointerRange<char>> imageBc1Encode(const Image *image, const ImageKtxEncodeConfig &config)
	{
		if (image->channels() != 3)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc1 encoding");
		const Image *imgs[1] = { image };
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc1;
		return std::move(imageKtxTranscode(imgs, config, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc3Encode(const Image *image, const ImageKtxEncodeConfig &config)
	{
		if (image->channels() != 4)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc3 encoding");
		const Image *imgs[1] = { image };
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc3;
		return std::move(imageKtxTranscode(imgs, config, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc4Encode(const Image *image, const ImageKtxEncodeConfig &config)
	{
		if (image->channels() != 1)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc4 encoding");
		const Image *imgs[1] = { image };
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc4;
		return std::move(imageKtxTranscode(imgs, config, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc5Encode(const Image *image, const ImageKtxEncodeConfig &config)
	{
		if (image->channels() != 2)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc5 encoding");
		const Image *imgs[1] = { image };
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc5;
		return std::move(imageKtxTranscode(imgs, config, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc7Encode(const Image *image, const ImageKtxEncodeConfig &config)
	{
		if (image->channels() != 3 && image->channels() != 4)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc7 encoding");
		const Image *imgs[1] = { image };
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc7;
		return std::move(imageKtxTranscode(imgs, config, cfg2)[0].data);
	}

	namespace
	{
		CAGE_FORCE_INLINE uint32 blockyIndex(uint32 x, uint32 y, uint32 width)
		{
			const uint32 bx = x / 4;
			const uint32 by = y / 4;
			const uint32 bw = width / 4;
			const uint32 block = by * bw + bx;
			const uint32 pixelInBlock = (y % 4) * 4 + (x % 4);
			return block * 16 + pixelInBlock;
		}

		Holder<Image> finalize(PointerRange<const basisu::color_rgba> colors, const Vec2i &resolution_, uint32 channels)
		{
			const Vec2i resolution = 4 * ((resolution_ + 3) / 4);
			CAGE_ASSERT(colors.size() == resolution[0] * resolution[1]);
			Holder<Image> img = newImage();
			img->initialize(resolution_, channels, ImageFormatEnum::U8);
			for (uint32 y = 0; y < resolution_[1]; y++)
				for (uint32 x = 0; x < resolution_[0]; x++)
					for (uint32 c = 0; c < channels; c++)
						img->value(x, y, c, colors[blockyIndex(x, y, resolution[0])][c] / 255.f);
			return img;
		}
	}

	Holder<Image> imageBc1Decode(PointerRange<const char> buffer, const Vec2i &resolution_)
	{
		const Vec2i resolution = 4 * ((resolution_ + 3) / 4);
		static constexpr uintPtr blockSize = 8;
		const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
		if (blocksRequired * blockSize > buffer.size())
			CAGE_THROW_ERROR(Exception, "insufficient data for bcn decoding");
		const uint32 pixelsCount = blocksRequired * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksRequired; bi++)
			basisu::unpack_bc1(buffer.data() + bi * blockSize, colors.data() + bi * 16, false);
		return finalize(colors, resolution_, 3);
	}

	namespace
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

		Vec3 unpack565(uint16 v)
		{
			uint8 x = (v >> 0) & 31u;
			uint8 y = (v >> 5) & 63u;
			uint8 z = (v >> 11) & 31u;
			return Vec3(x, y, z) * Vec3(1.0 / 31, 1.0 / 63, 1.0 / 31);
		}

		U8Block F32ToU8Block(const F32Block &in)
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

		void decompress(Deserializer &des, ImageImpl *impl)
		{
			Serializer masterSer(impl->mem);
			const uint32 cols = impl->width / 4; // blocks count
			const uint32 rows = impl->height / 4; // blocks count
			const uint32 lineSize = impl->width * 4; // bytes count
			for (uint32 y = 0; y < rows; y++)
			{
				Serializer tmpSer0 = masterSer.reserve(lineSize);
				Serializer tmpSer1 = masterSer.reserve(lineSize);
				Serializer tmpSer2 = masterSer.reserve(lineSize);
				Serializer tmpSer3 = masterSer.reserve(lineSize);
				Serializer *lineSer[4] = { &tmpSer0, &tmpSer1, &tmpSer2, &tmpSer3 };
				for (uint32 x = 0; x < cols; x++)
				{
					Bc2Block block;
					des >> block;
					U8Block texels;
					bcDecompress(block, texels);
					for (uint32 b = 0; b < 4; b++)
					{
						for (uint32 a = 0; a < 4; a++)
						{
							for (uint32 i = 0; i < 4; i++)
								*(lineSer[b]) << texels.texel[b][a][i];
						}
					}
				}
			}
			CAGE_ASSERT(masterSer.available() == 0);
		}
	}

	Holder<Image> imageBc2Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		Holder<Image> img = newImage();
		img->initialize(resolution, 4, ImageFormatEnum::U8);
		Deserializer des(buffer);
		decompress(des, (ImageImpl *)+img);
		return img;
	}

	Holder<Image> imageBc3Decode(PointerRange<const char> buffer, const Vec2i &resolution_)
	{
		const Vec2i resolution = 4 * ((resolution_ + 3) / 4);
		static constexpr uintPtr blockSize = 16;
		const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
		if (blocksRequired * blockSize > buffer.size())
			CAGE_THROW_ERROR(Exception, "insufficient data for bcn decoding");
		const uint32 pixelsCount = blocksRequired * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksRequired; bi++)
			basisu::unpack_bc3(buffer.data() + bi * blockSize, colors.data() + bi * 16);
		return finalize(colors, resolution_, 4);
	}

	Holder<Image> imageBc4Decode(PointerRange<const char> buffer, const Vec2i &resolution_)
	{
		const Vec2i resolution = 4 * ((resolution_ + 3) / 4);
		static constexpr uintPtr blockSize = 8;
		const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
		if (blocksRequired * blockSize > buffer.size())
			CAGE_THROW_ERROR(Exception, "insufficient data for bcn decoding");
		const uint32 pixelsCount = blocksRequired * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksRequired; bi++)
			basisu::unpack_bc4(buffer.data() + bi * blockSize, (uint8_t *)(colors.data() + bi * 16), 4);
		return finalize(colors, resolution_, 1);
	}

	Holder<Image> imageBc5Decode(PointerRange<const char> buffer, const Vec2i &resolution_)
	{
		const Vec2i resolution = 4 * ((resolution_ + 3) / 4);
		static constexpr uintPtr blockSize = 16;
		const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
		if (blocksRequired * blockSize > buffer.size())
			CAGE_THROW_ERROR(Exception, "insufficient data for bcn decoding");
		const uint32 pixelsCount = blocksRequired * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksRequired; bi++)
			basisu::unpack_bc5(buffer.data() + bi * blockSize, colors.data() + bi * 16);
		return finalize(colors, resolution_, 2);
	}

	Holder<Image> imageBc7Decode(PointerRange<const char> buffer, const Vec2i &resolution_)
	{
		const Vec2i resolution = 4 * ((resolution_ + 3) / 4);
		static constexpr uintPtr blockSize = 16;
		const uint32 blocksRequired = resolution[0] * resolution[1] / 16;
		if (blocksRequired * blockSize > buffer.size())
			CAGE_THROW_ERROR(Exception, "insufficient data for bcn decoding");
		const uint32 pixelsCount = blocksRequired * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksRequired; bi++)
			basisu::unpack_bc7(buffer.data() + bi * blockSize, colors.data() + bi * 16);
		return finalize(colors, resolution_, 4);
	}
}
