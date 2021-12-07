#include "image.h"

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
	}

	void ktxDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		basist::ktx2_transcoder trans(nullptr);

		if (!trans.init(inBuffer.data(), inBuffer.size()))
			CAGE_THROW_ERROR(Exception, "failed parsing ktx file header");
		if (!trans.start_transcoding())
			CAGE_THROW_ERROR(Exception, "failed loading ktx file data");

		impl->width = trans.get_width();
		impl->height = trans.get_height();
		impl->channels = 4;
		impl->format = ImageFormatEnum::U8;
		impl->mem.allocate(impl->width * impl->height * 4);
		if (!trans.transcode_image_level(0, 0, 0, impl->mem.data(), impl->width * impl->height, basist::transcoder_texture_format::cTFRGBA32))
			CAGE_THROW_ERROR(Exception, "failed decoding an image from ktx file");

		impl->colorConfig = defaultConfig(impl->channels);
	}

	MemoryBuffer ktxEncode(const ImageImpl *impl)
	{
		if (impl->channels > 4)
			CAGE_THROW_ERROR(Exception, "unsupported channels count for ktx encoding");
		if (impl->format != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for ktx encoding");

		initBasisuOnce();

		basisu::image img;
		img.init((const std::uint8_t *)impl->mem.data(), impl->width, impl->height, impl->channels);

		basisu::basis_compressor_params params;
		params.m_uastc = true;
		params.m_multithreading = false;
		params.m_rdo_uastc_multithreading = false;
		params.m_create_ktx2_file = true;
		params.m_ktx2_uastc_supercompression = basist::ktx2_supercompression::KTX2_SS_ZSTANDARD;
		params.m_source_images.push_back(img);

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
		MemoryBuffer buff(output.size());
		detail::memcpy(buff.data(), output.data(), buff.size());
		return buff;
	}
}
