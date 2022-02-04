#include "image.h"

#include <cage-core/imageBlocks.h>
#include <cage-core/imageImport.h>
#include <cage-core/pointerRangeHolder.h>

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

		basist::transcoder_texture_format convertFormat(ImageKtxTranscodeFormatEnum format)
		{
			switch (format)
			{
			case ImageKtxTranscodeFormatEnum::Bc1: return basist::transcoder_texture_format::cTFBC1_RGB;
			case ImageKtxTranscodeFormatEnum::Bc3: return basist::transcoder_texture_format::cTFBC3_RGBA;
			case ImageKtxTranscodeFormatEnum::Bc4: return basist::transcoder_texture_format::cTFBC4_R;
			case ImageKtxTranscodeFormatEnum::Bc5: return basist::transcoder_texture_format::cTFBC5_RG;
			case ImageKtxTranscodeFormatEnum::Bc7: return basist::transcoder_texture_format::cTFBC7_RGBA;
			case ImageKtxTranscodeFormatEnum::Astc: return basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
			}
			CAGE_THROW_ERROR(Exception, "invalid ImageKtxBlocksFormatEnum");
		}

		void throwError(basisu::basis_compressor::error_code result)
		{
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
		}

		basisu::basis_compressor_params defaultCompressorParams()
		{
			basisu::basis_compressor_params params;
			params.m_uastc = true;
			params.m_multithreading = false;
			params.m_rdo_uastc_multithreading = false;
			params.m_status_output = false;
			return params;
		}

		void pushImages(PointerRange<const Image *> images, basisu::basis_compressor_params &params)
		{
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
		}
	}

	Holder<PointerRange<char>> imageKtxEncode(PointerRange<const Image *> images, const ImageKtxEncodeConfig &config)
	{
		initBasisuOnce();

		basisu::basis_compressor_params params = defaultCompressorParams();
		params.m_create_ktx2_file = true;
		params.m_perceptual = images[0]->colorConfig.gammaSpace == GammaSpaceEnum::Gamma;
		params.m_ktx2_uastc_supercompression = basist::ktx2_supercompression::KTX2_SS_ZSTANDARD;
		pushImages(images, params);

		basisu::job_pool jpool(1);
		params.m_pJob_pool = &jpool;
		basisu::basis_compressor compressor;
		if (!compressor.init(params))
			CAGE_THROW_ERROR(Exception, "failed to initialize ktx compressor");
		throwError(compressor.process());

		const auto output = compressor.get_output_ktx2_file();
		PointerRangeHolder<char> buff;
		buff.resize(output.size());
		detail::memcpy(buff.data(), output.data(), buff.size());
		return buff;
	}

	Holder<PointerRange<Holder<Image>>> imageKtxDecode(PointerRange<const char> buffer)
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

	Holder<PointerRange<ImageKtxTranscodeResult>> imageKtxTranscode(PointerRange<const char> buffer, const ImageKtxTranscodeConfig &config)
	{
		initBasisuOnce();

		basist::ktx2_transcoder trans(nullptr);

		if (!trans.init(buffer.data(), buffer.size()))
			CAGE_THROW_ERROR(Exception, "failed parsing ktx file header");
		if (!trans.start_transcoding())
			CAGE_THROW_ERROR(Exception, "failed loading ktx file data");

		const basist::transcoder_texture_format format = convertFormat(config.format);
		const uint32 bytesPerBlock = basis_get_bytes_per_block_or_pixel(format);

		PointerRangeHolder<ImageKtxTranscodeResult> output;
		for (uint32 level = 0; level < trans.get_levels(); level++)
		{
			for (uint32 face = 0; face < trans.get_faces(); face++)
			{
				for (uint32 layer = 0; layer < max(uint32(1), trans.get_layers()); layer++)
				{
					basist::ktx2_image_level_info info;
					if (!trans.get_image_level_info(info, level, layer, face))
						CAGE_THROW_ERROR(Exception, "failed decoding image info from ktx file");
					ImageKtxTranscodeResult img;
					img.blocks = Vec2i(info.m_num_blocks_x, info.m_num_blocks_y);
					img.resolution = Vec2i(trans.get_width(), trans.get_height());
					img.mipmapLevel = level;
					img.cubeFace = face;
					img.layer = layer;
					PointerRangeHolder<char> buff;
					buff.resize(info.m_total_blocks * bytesPerBlock);
					if (!trans.transcode_image_level(level, layer, face, buff.data(), info.m_total_blocks, format))
						CAGE_THROW_ERROR(Exception, "failed decoding image pixels from ktx file");
					img.data = std::move(buff);
					output.push_back(std::move(img));
				}
			}
		}

		if (output.empty())
			CAGE_THROW_ERROR(Exception, "no images retrieved from ktx file");
		return output;
	}

	Holder<PointerRange<ImageKtxTranscodeResult>> imageKtxTranscode(PointerRange<const Image *> images, const ImageKtxEncodeConfig &compression, const ImageKtxTranscodeConfig &transcode)
	{
		initBasisuOnce();

		basisu::basis_compressor_params params = defaultCompressorParams();
		params.m_perceptual = images[0]->colorConfig.gammaSpace == GammaSpaceEnum::Gamma;
		pushImages(images, params);

		basisu::job_pool jpool(1);
		params.m_pJob_pool = &jpool;
		basisu::basis_compressor compressor;
		if (!compressor.init(params))
			CAGE_THROW_ERROR(Exception, "failed to initialize basisu compressor");
		throwError(compressor.process());

		const auto intermediate = compressor.get_output_basis_file();
		basist::basisu_transcoder trans(nullptr);
		if (!trans.start_transcoding(intermediate.data(), intermediate.size()))
			CAGE_THROW_ERROR(Exception, "failed loading basisu file data");

		const basist::transcoder_texture_format format = convertFormat(transcode.format);
		const uint32 bytesPerBlock = basis_get_bytes_per_block_or_pixel(format);

		PointerRangeHolder<ImageKtxTranscodeResult> output;
		const uint32 totalLayers = trans.get_total_images(intermediate.data(), intermediate.size());
		for (uint32 layer = 0; layer < totalLayers; layer++)
		{
			const uint32 totalLevels = trans.get_total_image_levels(intermediate.data(), intermediate.size(), layer);
			for (uint32 level = 0; level < totalLevels; level++)
			{
				basist::basisu_image_level_info info;
				if (!trans.get_image_level_info(intermediate.data(), intermediate.size(), info, layer, level))
					CAGE_THROW_ERROR(Exception, "failed decoding image info from basisu file");
				ImageKtxTranscodeResult img;
				img.blocks = Vec2i(info.m_num_blocks_x, info.m_num_blocks_y);
				img.resolution = Vec2i(info.m_orig_width, info.m_orig_height);
				img.mipmapLevel = level;
				img.layer = layer;
				PointerRangeHolder<char> buff;
				buff.resize(info.m_total_blocks * bytesPerBlock);
				if (!trans.transcode_image_level(intermediate.data(), intermediate.size(), layer, level, buff.data(), info.m_total_blocks, format))
					CAGE_THROW_ERROR(Exception, "failed decoding image pixels from basisu file");
				img.data = std::move(buff);
				output.push_back(std::move(img));
			}
		}

		if (output.empty())
			CAGE_THROW_ERROR(Exception, "no images retrieved from basisu file");
		return output;
	}

	void ktxDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		Holder<PointerRange<Holder<Image>>> res = imageKtxDecode(inBuffer);
		swapAll(impl, (ImageImpl *)+res[0]);
	}

	ImageImportResult ktxDecode(PointerRange<const char> inBuffer, const ImageImportConfig &config)
	{
		ImageKtxTranscodeConfig ktxConfig;
		ktxConfig.format = ImageKtxTranscodeFormatEnum::Bc1; // todo choose depending on the file
		Holder<PointerRange<ImageKtxTranscodeResult>> ktx = imageKtxTranscode(inBuffer, ktxConfig);
		PointerRangeHolder<ImageImportPart> parts;
		for (auto &it : ktx)
		{
			ImageImportRaw raw;
			raw.format = "bc1"; // todo
			raw.data = std::move(it.data);
			raw.resolution = it.resolution;
			raw.blocks = it.blocks;
			raw.channels = 3;
			ImageImportPart part;
			part.raw = systemMemory().createHolder<ImageImportRaw>(std::move(raw));
			part.mipmapLevel = it.mipmapLevel;
			part.cubeFace = it.cubeFace;
			part.layer = it.layer;
			parts.push_back(std::move(part));
		}
		ImageImportResult result;
		result.parts = std::move(parts);
		return result;
	}

	MemoryBuffer ktxEncode(const ImageImpl *impl)
	{
		const Image *arr[1] = { impl };
		auto res = imageKtxEncode(arr, {});
		MemoryBuffer buff;
		buff.resize(res.size());
		detail::memcpy(buff.data(), res.data(), buff.size());
		return buff;
	}
}
