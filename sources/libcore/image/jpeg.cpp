#include <stdio.h> // needed for jpeglib

#include <jpeglib.h>

#include "image.h"

namespace cage
{
	namespace
	{
		void jpegErrFunc(j_common_ptr cinfo)
		{
			char jpegLastErrorMsg[JMSG_LENGTH_MAX];
			(*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
			CAGE_THROW_ERROR(Exception, jpegLastErrorMsg);
		}

		void jpegDecode(const char *inBuffer, uintPtr inSize, MemoryBuffer &out, uint32 &width, uint32 &height, uint32 &components)
		{
			jpeg_decompress_struct info;
			jpeg_error_mgr errmgr;
			info.err = jpeg_std_error(&errmgr);
			errmgr.error_exit = &jpegErrFunc;
			try
			{
				jpeg_create_decompress(&info);
				jpeg_mem_src(&info, (unsigned char *)inBuffer, numeric_cast<uint32>(inSize));
				jpeg_read_header(&info, TRUE);
				jpeg_start_decompress(&info);
				width = info.output_width;
				height = info.output_height;
				components = info.num_components;
				uint32 lineSize = components * width;
				out.allocate(lineSize * height);
				while (info.output_scanline < info.output_height)
				{
					unsigned char *ptr[1];
					ptr[0] = (unsigned char *)out.data() + lineSize * info.output_scanline;
					jpeg_read_scanlines(&info, ptr, 1);
				}
				jpeg_finish_decompress(&info);
				jpeg_destroy_decompress(&info);
			}
			catch (...)
			{
				jpeg_destroy_decompress(&info);
				throw;
			}
		}

		void jpegInitDestination(j_compress_ptr cinfo);
		boolean jpegEmptyOutputBuffer(j_compress_ptr cinfo);
		void jpegTermDestination(j_compress_ptr cinfo);

		struct JpegWriteCtx : public jpeg_destination_mgr
		{
			MemoryBuffer &buf;
			JpegWriteCtx(MemoryBuffer &buf) : buf(buf)
			{
				buf.resize(0);
				init_destination = &jpegInitDestination;
				empty_output_buffer = &jpegEmptyOutputBuffer;
				term_destination = &jpegTermDestination;
			}
		};

		constexpr uintPtr blockSize = 16384;

		void jpegInitDestination(j_compress_ptr cinfo)
		{
			JpegWriteCtx *ctx = (JpegWriteCtx *)cinfo->dest;
			ctx->buf.resizeSmart(blockSize);
			ctx->next_output_byte = (JOCTET *)ctx->buf.data();
			ctx->free_in_buffer = ctx->buf.size();
		}

		boolean jpegEmptyOutputBuffer(j_compress_ptr cinfo)
		{
			JpegWriteCtx *ctx = (JpegWriteCtx *)cinfo->dest;
			uintPtr oldsize = ctx->buf.size();
			ctx->buf.resizeSmart(oldsize + blockSize);
			ctx->next_output_byte = (JOCTET *)(ctx->buf.data() + oldsize);
			ctx->free_in_buffer = ctx->buf.size() - oldsize;
			return true;
		}

		void jpegTermDestination(j_compress_ptr cinfo)
		{
			JpegWriteCtx *ctx = (JpegWriteCtx *)cinfo->dest;
			ctx->buf.resize(ctx->buf.size() - ctx->free_in_buffer);
		}

		void jpegEncode(const MemoryBuffer &in, MemoryBuffer &out, uint32 width, uint32 height, uint32 components)
		{
			jpeg_compress_struct info;
			jpeg_error_mgr errmgr;
			info.err = jpeg_std_error(&errmgr);
			errmgr.error_exit = &jpegErrFunc;
			JpegWriteCtx writectx((out));
			try
			{
				jpeg_create_compress(&info);
				info.dest = &writectx;
				info.image_width = width;
				info.image_height = height;
				info.input_components = components;
				info.in_color_space = JCS_UNKNOWN;
				switch (components)
				{
					case 1:
						info.in_color_space = JCS_GRAYSCALE;
						break;
					case 3:
						info.in_color_space = JCS_RGB;
						break;
				}
				jpeg_set_defaults(&info);
				jpeg_start_compress(&info, true);
				uint32 lineSize = components * width;
				while (info.next_scanline < info.image_height)
				{
					unsigned char *ptr[1];
					ptr[0] = (unsigned char *)in.data() + lineSize * info.next_scanline;
					jpeg_write_scanlines(&info, ptr, 1);
				}
				jpeg_finish_compress(&info);
				jpeg_destroy_compress(&info);
			}
			catch (...)
			{
				jpeg_destroy_compress(&info);
				throw;
			}
		}
	}

	void jpegDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		jpegDecode(inBuffer.data(), inBuffer.size(), impl->mem, impl->width, impl->height, impl->channels);
		impl->format = ImageFormatEnum::U8;
		impl->colorConfig = defaultConfig(impl->channels);
	}

	MemoryBuffer jpegEncode(const ImageImpl *impl)
	{
		if (impl->channels != 1 && impl->channels != 3)
			CAGE_THROW_ERROR(Exception, "unsupported channels count for jpeg encoding");
		if (impl->format != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for jpeg encoding");

		MemoryBuffer res;
		res.reserve(impl->width * impl->height * impl->channels + 100);
		jpegEncode(impl->mem, res, impl->width, impl->height, impl->channels);
		return res;
	}
}
