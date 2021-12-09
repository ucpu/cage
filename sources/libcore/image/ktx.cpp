#include "image.h"

#include <cage-core/imageKtx.h>
#include <cage-core/pointerRangeHolder.h>

#define BASISU_NO_ITERATOR_DEBUG_LEVEL
#include <basis_universal/encoder/basisu_comp.h>
#include <basis_universal/transcoder/basisu_transcoder.h>

namespace cage
{
	namespace
	{
		void initBasisuOnce()
		{
			static int dummy = [](){ basisu::basisu_encoder_init(); return 0; }();
		}

		basist::transcoder_texture_format convertFormat(ImageKtxBlocksFormatEnum format)
		{
			switch (format)
			{
			case ImageKtxBlocksFormatEnum::Bc1: return basist::transcoder_texture_format::cTFBC1_RGB;
			case ImageKtxBlocksFormatEnum::Bc3: return basist::transcoder_texture_format::cTFBC3_RGBA;
			case ImageKtxBlocksFormatEnum::Bc4: return basist::transcoder_texture_format::cTFBC4_R;
			case ImageKtxBlocksFormatEnum::Bc5: return basist::transcoder_texture_format::cTFBC5_RG;
			case ImageKtxBlocksFormatEnum::Bc7: return basist::transcoder_texture_format::cTFBC7_RGBA;
			case ImageKtxBlocksFormatEnum::Astc: return basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
			}
			CAGE_THROW_ERROR(Exception, "invalid ImageKtxBlocksFormatEnum");
		}
	}

	Holder<PointerRange<char>> imageKtxCompress(PointerRange<const Image *> images, const ImageKtxCompressionConfig &config)
	{
		initBasisuOnce();

		basisu::basis_compressor_params params;
		params.m_uastc = true;
		params.m_multithreading = false;
		params.m_rdo_uastc_multithreading = false;
		params.m_create_ktx2_file = true;
		params.m_status_output = false;
		params.m_perceptual = images[0]->colorConfig.gammaSpace == GammaSpaceEnum::Gamma;
		params.m_ktx2_uastc_supercompression = basist::ktx2_supercompression::KTX2_SS_ZSTANDARD;

		for (const Image *src : images)
		{
			if (src->channels() > 4)
				CAGE_THROW_ERROR(Exception, "unsupported channels count for ktx encoding");
			if (src->format() != ImageFormatEnum::U8)
				CAGE_THROW_ERROR(Exception, "unsupported image format for ktx encoding");
			basisu::image img;
			img.init((const std::uint8_t *)((const ImageImpl *)src)->mem.data(), src->width(), src->height(), src->channels());
			params.m_source_images.push_back(std::move(img));
		}

		basisu::job_pool jpool(1);
		params.m_pJob_pool = &jpool;
		basisu::basis_compressor compressor;
		if (!compressor.init(params))
			CAGE_THROW_ERROR(Exception, "failed to initialize ktx compressor");

		const basisu::basis_compressor::error_code result = compressor.process();
		if (result != basisu::basis_compressor::error_code::cECSuccess)
		{
			switch (result)
			{
			case basisu::basis_compressor::error_code::cECFailedReadingSourceImages: CAGE_LOG_THROW("failed loading source images"); break;
			case basisu::basis_compressor::error_code::cECFailedValidating: CAGE_LOG_THROW("failed validation"); break;
			case basisu::basis_compressor::error_code::cECFailedEncodeUASTC: CAGE_LOG_THROW("failed encoding uastc"); break;
			case basisu::basis_compressor::error_code::cECFailedFrontEnd: CAGE_LOG_THROW("failed frontend"); break;
			case basisu::basis_compressor::error_code::cECFailedFontendExtract: CAGE_LOG_THROW("failed frontend extract"); break;
			case basisu::basis_compressor::error_code::cECFailedBackend: CAGE_LOG_THROW("failed backend"); break;
			case basisu::basis_compressor::error_code::cECFailedCreateBasisFile: CAGE_LOG_THROW("failed to create basis file"); break;
			case basisu::basis_compressor::error_code::cECFailedWritingOutput: CAGE_LOG_THROW("failed writing output"); break;
			case basisu::basis_compressor::error_code::cECFailedUASTCRDOPostProcess: CAGE_LOG_THROW("failed uastc rdo post process"); break;
			case basisu::basis_compressor::error_code::cECFailedCreateKTX2File: CAGE_LOG_THROW("failed to create ktx file"); break;
			}
			CAGE_THROW_ERROR(Exception, "failed encoding ktx image");
		}

		const auto output = compressor.get_output_ktx2_file();
		PointerRangeHolder<char> buff;
		buff.resize(output.size());
		detail::memcpy(buff.data(), output.data(), buff.size());
		return buff;
	}

	Holder<PointerRange<Holder<Image>>> imageKtxDecompressRaw(PointerRange<const char> buffer)
	{
		initBasisuOnce();

		basist::ktx2_transcoder trans(nullptr);

		if (!trans.init(buffer.data(), buffer.size()))
			CAGE_THROW_ERROR(Exception, "failed parsing ktx file header");
		if (!trans.start_transcoding())
			CAGE_THROW_ERROR(Exception, "failed loading ktx file data");

		PointerRangeHolder<Holder<Image>> images;

		for (uint32 level = 0; level < trans.get_levels(); level++)
		{
			for (uint32 face = 0; face < trans.get_faces(); face++)
			{
				for (uint32 layer = 0; layer < max(uint32(1), trans.get_layers()); layer++)
				{
					Holder<Image> img = newImage();
					img->initialize(trans.get_width(), trans.get_height(), 4, ImageFormatEnum::U8);
					if (!trans.transcode_image_level(level, layer, face, ((ImageImpl *)+img)->mem.data(), img->width() * img->height(), basist::transcoder_texture_format::cTFRGBA32))
						CAGE_THROW_ERROR(Exception, "failed decoding an image from ktx file");
					img->colorConfig = defaultConfig(img->channels());
					images.push_back(std::move(img));
				}
			}
		}

		if (images.empty())
			CAGE_THROW_ERROR(Exception, "no images retrieved from ktx file");
		return images;
	}

	Holder<PointerRange<ImageKtxDecompressionResult>> imageKtxDecompressBlocks(PointerRange<const char> buffer, const ImageKtxDecompressionConfig &config)
	{
		initBasisuOnce();

		basist::ktx2_transcoder trans(nullptr);

		if (!trans.init(buffer.data(), buffer.size()))
			CAGE_THROW_ERROR(Exception, "failed parsing ktx file header");
		if (!trans.start_transcoding())
			CAGE_THROW_ERROR(Exception, "failed loading ktx file data");

		const basist::transcoder_texture_format format = convertFormat(config.format);
		const uint32 bytesPerBlock = basis_get_bytes_per_block_or_pixel(format);

		PointerRangeHolder<ImageKtxDecompressionResult> images;

		for (uint32 level = 0; level < trans.get_levels(); level++)
		{
			for (uint32 face = 0; face < trans.get_faces(); face++)
			{
				for (uint32 layer = 0; layer < max(uint32(1), trans.get_layers()); layer++)
				{
					basist::ktx2_image_level_info info;
					if (!trans.get_image_level_info(info, level, layer, face))
						CAGE_THROW_ERROR(Exception, "failed decoding image info from ktx file");
					ImageKtxDecompressionResult img;
					img.blocks = Vec2i(info.m_num_blocks_x, info.m_num_blocks_y);
					img.resolution = Vec2i(trans.get_width(), trans.get_height());
					PointerRangeHolder<char> buff;
					buff.resize(info.m_total_blocks * bytesPerBlock);
					if (!trans.transcode_image_level(level, layer, face, buff.data(), info.m_total_blocks, format))
						CAGE_THROW_ERROR(Exception, "failed decoding image pixels from ktx file");
					img.data = std::move(buff);
					images.push_back(std::move(img));
				}
			}
		}

		if (images.empty())
			CAGE_THROW_ERROR(Exception, "no images retrieved from ktx file");
		return images;
	}

	void ktxDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		Holder<PointerRange<Holder<Image>>> res = imageKtxDecompressRaw(inBuffer);
		swapAll(impl, (ImageImpl *)+res[0]);
	}

	MemoryBuffer ktxEncode(const ImageImpl *impl)
	{
		const Image *arr[1] = { impl };
		auto res = imageKtxCompress(arr, {});
		MemoryBuffer buff;
		buff.resize(res.size());
		detail::memcpy(buff.data(), res.data(), buff.size());
		return buff;
	}
}
