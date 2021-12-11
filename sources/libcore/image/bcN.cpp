#include <cage-core/imageBlocks.h>

#include <basis_universal/encoder/basisu_gpu_texture.h>

#include <vector>

namespace cage
{
	Holder<PointerRange<char>> imageBc1Encode(const Image *image)
	{
		if (image->channels() != 3)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc1 encoding");
		const Image *imgs[1] = { image };
		ImageKtxEncodeConfig cfg1;
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc1;
		return std::move(imageKtxTranscode(imgs, cfg1, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc3Encode(const Image *image)
	{
		if (image->channels() != 4)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc3 encoding");
		const Image *imgs[1] = { image };
		ImageKtxEncodeConfig cfg1;
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc3;
		return std::move(imageKtxTranscode(imgs, cfg1, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc4Encode(const Image *image)
	{
		if (image->channels() != 1)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc4 encoding");
		const Image *imgs[1] = { image };
		ImageKtxEncodeConfig cfg1;
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc4;
		return std::move(imageKtxTranscode(imgs, cfg1, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc5Encode(const Image *image)
	{
		if (image->channels() != 2)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc5 encoding");
		const Image *imgs[1] = { image };
		ImageKtxEncodeConfig cfg1;
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc5;
		return std::move(imageKtxTranscode(imgs, cfg1, cfg2)[0].data);
	}

	Holder<PointerRange<char>> imageBc7Encode(const Image *image)
	{
		if (image->channels() != 4)
			CAGE_THROW_ERROR(Exception, "invalid number of channels for bc7 encoding");
		const Image *imgs[1] = { image };
		ImageKtxEncodeConfig cfg1;
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = ImageKtxTranscodeFormatEnum::Bc7;
		return std::move(imageKtxTranscode(imgs, cfg1, cfg2)[0].data);
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

		Holder<Image> finalize(PointerRange<const basisu::color_rgba> colors, const Vec2i &resolution, uint32 channels)
		{
			if (colors.size() != resolution[0] * resolution[1])
				CAGE_THROW_ERROR(Exception, "invalid resolution for bcN decoding");
			Holder<Image> img = newImage();
			img->initialize(resolution, channels, ImageFormatEnum::U8);
			for (uint32 y = 0; y < resolution[1]; y++)
				for (uint32 x = 0; x < resolution[0]; x++)
					for (uint32 c = 0; c < channels; c++)
						img->value(x, y, c, colors[blockyIndex(x, y, resolution[0])][c] / 255.f);
			return img;
		}
	}

	Holder<Image> imageBc1Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		constexpr uint32 blockSize = 8;
		const uint32 blocksCount = buffer.size() / blockSize;
		const uint32 pixelsCount = blocksCount * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksCount; bi++)
			basisu::unpack_bc1(buffer.data() + bi * blockSize, colors.data() + bi * 16, false);
		return finalize(colors, resolution, 3);
	}

	Holder<Image> imageBc2Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		CAGE_THROW_ERROR(NotImplemented, "bc2 image decoding is not implemented");
	}

	Holder<Image> imageBc3Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		constexpr uint32 blockSize = 16;
		const uint32 blocksCount = buffer.size() / blockSize;
		const uint32 pixelsCount = blocksCount * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksCount; bi++)
			basisu::unpack_bc3(buffer.data() + bi * blockSize, colors.data() + bi * 16);
		return finalize(colors, resolution, 4);
	}

	Holder<Image> imageBc4Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		constexpr uint32 blockSize = 8;
		const uint32 blocksCount = buffer.size() / blockSize;
		const uint32 pixelsCount = blocksCount * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksCount; bi++)
			basisu::unpack_bc4(buffer.data() + bi * blockSize, (uint8_t *)(colors.data() + bi * 16), 4);
		return finalize(colors, resolution, 1);
	}

	Holder<Image> imageBc5Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		constexpr uint32 blockSize = 16;
		const uint32 blocksCount = buffer.size() / blockSize;
		const uint32 pixelsCount = blocksCount * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksCount; bi++)
			basisu::unpack_bc5(buffer.data() + bi * blockSize, colors.data() + bi * 16);
		return finalize(colors, resolution, 2);
	}

	Holder<Image> imageBc7Decode(PointerRange<const char> buffer, const Vec2i &resolution)
	{
		constexpr uint32 blockSize = 16;
		const uint32 blocksCount = buffer.size() / blockSize;
		const uint32 pixelsCount = blocksCount * 16;
		std::vector<basisu::color_rgba> colors;
		colors.resize(pixelsCount);
		for (uint32 bi = 0; bi < blocksCount; bi++)
			basisu::unpack_bc7(buffer.data() + bi * blockSize, colors.data() + bi * 16);
		return finalize(colors, resolution, 4);
	}
}
