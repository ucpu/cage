#include "image.h"

#include <cage-core/endianness.h>
#include <png.h>
#include <vector>

namespace cage
{
	namespace
	{
		void pngErrFunc(png_structp, png_const_charp err)
		{
			CAGE_THROW_ERROR(Exception, err);
		}

		struct PngInfoCtx
		{
			png_structp png;
			png_infop info;
			bool writing;

			PngInfoCtx() : png(nullptr), info(nullptr), writing(false) {}
			~PngInfoCtx()
			{
				if (info)
					png_destroy_info_struct(png, &info);
				if (png)
				{
					if (writing)
						png_destroy_write_struct(&png, nullptr);
					else
						png_destroy_read_struct(&png, nullptr, nullptr);
				}
			}
		};

		void pngFlushFunc(png_structp png)
		{
			// do nothing
		}

		struct PngReadCtx
		{
			const char *start = nullptr;
			uintPtr size = 0;
			uintPtr off = 0;
		};

		void pngReadFunc(png_structp png, png_bytep buf, png_size_t siz)
		{
			PngReadCtx *io = (PngReadCtx*)png_get_io_ptr(png);
			if (io->off + siz > io->size)
				png_error(png, "png reading outside memory buffer");
			detail::memcpy(buf, io->start + io->off, siz);
			io->off += siz;
		}

		void pngDecode(const char *inBuffer, uintPtr inSize, MemoryBuffer &out, uint32 &width, uint32 &height, uint32 &components, uint32 &bpp)
		{
			PngInfoCtx ctx;
			png_structp &png = ctx.png;
			png_infop &info = ctx.info;
			png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, &pngErrFunc, nullptr);
			if (!png)
				CAGE_THROW_ERROR(Exception, "png decoder failed (png_structp)");
			PngReadCtx ioCtx;
			ioCtx.start = inBuffer;
			ioCtx.size = inSize;
			png_set_read_fn(png, &ioCtx, &pngReadFunc);
			info = png_create_info_struct(png);
			if (!info)
				CAGE_THROW_ERROR(Exception, "png decoder failed (png_infop)");
			png_read_info(png, info);
			width = png_get_image_width(png, info);
			height = png_get_image_height(png, info);
			png_byte colorType = png_get_color_type(png, info);
			png_byte bitDepth  = png_get_bit_depth(png, info);

			png_set_expand(png);
			if (endianness::little())
				png_set_swap(png);

			png_read_update_info(png, info);
			colorType = png_get_color_type(png, info);
			bitDepth  = png_get_bit_depth(png, info);
			switch (bitDepth)
			{
			case 8:
				bpp = 1;
				break;
			case 16:
				bpp = 2;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "png decoder failed (unsupported bit depth)");
			}
			switch (colorType)
			{
			case PNG_COLOR_TYPE_GRAY:
				components = 1;
				break;
			case PNG_COLOR_TYPE_GA:
				components = 2;
				break;
			case PNG_COLOR_TYPE_RGB:
				components = 3;
				break;
			case PNG_COLOR_TYPE_RGBA:
				components = 4;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "png decoder failed (unsupported color type)");
			}

			std::vector<png_bytep> rows;
			rows.resize(height);
			uint32 cols = width * components * bpp;
			CAGE_ASSERT(cols == png_get_rowbytes(png, info));
			out.allocate(height * cols);
			for (uint32 y = 0; y < height; y++)
				rows[y] = (png_bytep)out.data() + y * cols;
			png_read_image(png, rows.data());
		}

		struct PngWriteCtx
		{
			MemoryBuffer &buf;
			uintPtr off = 0;
			PngWriteCtx(MemoryBuffer &buf) : buf(buf)
			{
				buf.resize(0);
			}
		};

		void pngWriteFunc(png_structp png, png_bytep buf, png_size_t siz)
		{
			PngWriteCtx *io = (PngWriteCtx*)png_get_io_ptr(png);
			io->buf.resizeSmart(io->off + siz);
			detail::memcpy(io->buf.data() + io->off, buf, siz);
			io->off += siz;
		}

		void pngEncode(const MemoryBuffer &in, MemoryBuffer &out, uint32 width, uint32 height, uint32 components, uint32 bpp)
		{
			PngInfoCtx ctx;
			ctx.writing = true;
			png_structp &png = ctx.png;
			png_infop &info = ctx.info;
			png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, &pngErrFunc, nullptr);
			if (!png)
				CAGE_THROW_ERROR(Exception, "png encoder failed (png_structp)");
			PngWriteCtx ioCtx(out);
			png_set_write_fn(png, &ioCtx, &pngWriteFunc, &pngFlushFunc);

			if (endianness::little())
				png_set_swap(png);
			info = png_create_info_struct(png);
			if (!info)
				CAGE_THROW_ERROR(Exception, "png encoder failed (png_infop)");

			int colorType;
			switch (components)
			{
			case 1:
				colorType = PNG_COLOR_TYPE_GRAY;
				break;
			case 2:
				colorType = PNG_COLOR_TYPE_GA;
				break;
			case 3:
				colorType = PNG_COLOR_TYPE_RGB;
				break;
			case 4:
				colorType = PNG_COLOR_TYPE_RGBA;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "png encoder failed (unsupported color type)");
			}
			png_set_IHDR(png, info, width, height, bpp * 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			png_write_info(png, info);
			if (endianness::little())
				png_set_swap(png);

			std::vector<png_bytep> rows;
			rows.resize(height);
			uint32 cols = width * components * bpp;
			CAGE_ASSERT(cols == png_get_rowbytes(png, info));
			for (uint32 y = 0; y < height; y++)
				rows[y] = (png_bytep)in.data() + y * cols;
			png_write_image(png, rows.data());
			png_write_end(png, info);
		}
	}

	void pngDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl)
	{
		uint32 bpp = 0;
		pngDecode(inBuffer, inSize, impl->mem, impl->width, impl->height, impl->channels, bpp);
		switch (bpp)
		{
		case 1:
			impl->format = ImageFormatEnum::U8;
			break;
		case 2:
			impl->format = ImageFormatEnum::U16;
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported png image bpp");
		}
		impl->colorConfig = defaultConfig(impl->channels);
	}

	MemoryBuffer pngEncode(ImageImpl *impl)
	{
		if (impl->channels > 4)
			CAGE_THROW_ERROR(Exception, "unsupported channels count for png encoding");
		MemoryBuffer res;
		res.reserve(impl->width * impl->height * impl->channels * formatBytes(impl->format) + 100);
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
			pngEncode(impl->mem, res, impl->width, impl->height, impl->channels, 1);
			break;
		case ImageFormatEnum::U16:
			pngEncode(impl->mem, res, impl->width, impl->height, impl->channels, 2);
			break;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported format for png encoding");
		}
		return res;
	}
}
