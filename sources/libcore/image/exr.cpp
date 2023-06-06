#include "image.h"

#include <cage-core/serialization.h> // bufferCast
#include <stdlib.h> // free
#include <tinyexr.h>
#include <vector>

namespace cage
{
	void exrDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		const PointerRange<const unsigned char> buff = bufferCast<const unsigned char>(inBuffer);

		EXRVersion exr_version;
		if (ParseEXRVersionFromMemory(&exr_version, buff.data(), buff.size()) != 0)
			CAGE_THROW_ERROR(Exception, "failed to load version from exr image");

		EXRHeader exr_header;
		InitEXRHeader(&exr_header);
		if (ParseEXRHeaderFromMemory(&exr_header, &exr_version, buff.data(), buff.size(), nullptr) != 0)
			CAGE_THROW_ERROR(Exception, "failed to load header table from exr image");
		struct FreeHeader
		{
			EXRHeader *exr_header = m;
			FreeHeader(EXRHeader *exr_header) : exr_header(exr_header) {}
			~FreeHeader() { FreeEXRHeader(exr_header); }
		} freeHeader(&exr_header);

		for (int i = 0; i < exr_header.num_channels; i++)
		{
			switch (exr_header.pixel_types[i])
			{
				case TINYEXR_PIXELTYPE_FLOAT:
					break;
				case TINYEXR_PIXELTYPE_HALF:
					// convert half floats to actual floats
					exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
					break;
				default:
					CAGE_THROW_ERROR(Exception, "unsupported pixel type in loading exr image");
			}
		}

		EXRImage exr_image;
		InitEXRImage(&exr_image);
		if (LoadEXRImageFromMemory(&exr_image, &exr_header, buff.data(), buff.size(), nullptr) != 0)
			CAGE_THROW_ERROR(Exception, "failed to load exr image");
		struct FreeImage
		{
			EXRImage *exr_image = m;
			FreeImage(EXRImage *exr_image) : exr_image(exr_image) {}
			~FreeImage() { FreeEXRImage(exr_image); }
		} freeImage(&exr_image);

		impl->width = numeric_cast<uint32>(exr_image.width);
		impl->height = numeric_cast<uint32>(exr_image.height);
		impl->channels = numeric_cast<uint32>(exr_header.num_channels);
		impl->format = ImageFormatEnum::Float;
		impl->mem.allocate(impl->width * impl->height * impl->channels * sizeof(float));

		if (exr_image.tiles)
			CAGE_THROW_ERROR(Exception, "tiled exr images are not yet supported");
		CAGE_ASSERT(exr_image.images);

		// assume reverse channels order (ABGR)
		float *out_rgba = (float *)impl->mem.data();
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				for (uint32 c = 0; c < impl->channels; c++)
					*out_rgba++ = ((float **)exr_image.images)[impl->channels - c - 1][y * impl->width + x];
		CAGE_ASSERT((char *)out_rgba == impl->mem.data() + impl->mem.size());

		impl->colorConfig = defaultConfig(impl->channels);
		impl->colorConfig.gammaSpace = exr_header.channels[exr_header.num_channels - 1].p_linear ? GammaSpaceEnum::Linear : GammaSpaceEnum::Gamma;
	}

	MemoryBuffer exrEncode(const ImageImpl *impl)
	{
		if (impl->format != ImageFormatEnum::Float)
			CAGE_THROW_ERROR(Exception, "saving exr image requires float format");

		EXRHeader exr_header;
		InitEXRHeader(&exr_header);
		EXRImage exr_image;
		InitEXRImage(&exr_image);

		exr_header.compression_type = impl->width < 16 && impl->height < 16 ? TINYEXR_COMPRESSIONTYPE_NONE : TINYEXR_COMPRESSIONTYPE_ZIP;

		exr_header.num_channels = impl->channels;
		std::vector<EXRChannelInfo> channelInfos;
		channelInfos.resize(impl->channels);
		for (auto &it : channelInfos)
			detail::memset(&it, 0, sizeof(it));
		switch (impl->channels)
		{
			case 1:
				channelInfos[0].name[0] = 'A';
				break;
			case 2:
				channelInfos[0].name[0] = 'A';
				channelInfos[1].name[0] = 'R';
				channelInfos[1].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				break;
			case 3:
				channelInfos[0].name[0] = 'B';
				channelInfos[1].name[0] = 'G';
				channelInfos[2].name[0] = 'R';
				channelInfos[0].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				channelInfos[1].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				channelInfos[2].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				break;
			case 4:
				channelInfos[0].name[0] = 'A';
				channelInfos[1].name[0] = 'B';
				channelInfos[2].name[0] = 'G';
				channelInfos[3].name[0] = 'R';
				channelInfos[1].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				channelInfos[2].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				channelInfos[3].p_linear = impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear;
				break;
		}
		exr_header.channels = channelInfos.data();

		std::vector<int> pixelTypes;
		pixelTypes.resize(impl->channels, TINYEXR_PIXELTYPE_FLOAT);
		exr_header.pixel_types = pixelTypes.data();
		exr_header.requested_pixel_types = pixelTypes.data();

		exr_image.num_channels = impl->channels;
		exr_image.width = impl->width;
		exr_image.height = impl->height;

		std::vector<std::vector<float>> data;
		data.resize(impl->channels);
		const float *src = (const float *)impl->mem.data();
		for (uint32 c = 0; c < impl->channels; c++)
		{
			const uint32 rc = impl->channels - c - 1;
			data[c].reserve(impl->width * impl->height);
			for (uint32 y = 0; y < impl->height; y++)
				for (uint32 x = 0; x < impl->width; x++)
					data[c].push_back(src[((y * impl->width) + x) * impl->channels + rc]);
		}
		std::vector<unsigned char *> dataPtr;
		dataPtr.resize(impl->channels);
		for (uint32 c = 0; c < impl->channels; c++)
			dataPtr[c] = (unsigned char *)data[c].data();
		exr_image.images = dataPtr.data();

		unsigned char *mem = nullptr;
		std::size_t size = SaveEXRImageToMemory(&exr_image, &exr_header, &mem, nullptr);
		if (size == 0)
			CAGE_THROW_ERROR(Exception, "failed to save exr image");
		struct FreeMem
		{
			unsigned char *mem = m;
			FreeMem(unsigned char *mem) : mem(mem) {}
			~FreeMem() { free(mem); }
		} freeMem(mem);

		MemoryBuffer buff;
		buff.allocate(size);
		detail::memcpy(buff.data(), mem, size);
		return buff;
	}
}
