#include <cage-core/math.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/image.h>
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

		struct PngIoCtx
		{
			MemoryBuffer &buf;
			uintPtr off;
			PngIoCtx(MemoryBuffer &buf) : buf(buf), off(0) {}
		};

		void pngReadFunc(png_structp png, png_bytep buf, png_size_t siz)
		{
			PngIoCtx *io = (PngIoCtx*)png_get_io_ptr(png);
			if (io->off + siz > io->buf.size())
				png_error(png, "png reading outside memory buffer");
			detail::memcpy(buf, io->buf.data() + io->off, siz);
			io->off += siz;
		}

		void pngWriteFunc(png_structp png, png_bytep buf, png_size_t siz)
		{
			PngIoCtx *io = (PngIoCtx*)png_get_io_ptr(png);
			io->buf.resizeSmart(io->off + siz);
			detail::memcpy(io->buf.data() + io->off, buf, siz);
			io->off += siz;
		}

		void pngFlushFunc(png_structp png)
		{
			// do nothing
		}

		void decodePng(const MemoryBuffer &in, MemoryBuffer &out, uint32 &width, uint32 &height, uint32 &components, uint32 &bpp)
		{
			PngInfoCtx ctx;
			png_structp &png = ctx.png;
			png_infop &info = ctx.info;
			png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, &pngErrFunc, nullptr);
			if (!png)
				CAGE_THROW_ERROR(Exception, "png decoder failed (png_structp)");
			PngIoCtx ioCtx(const_cast<MemoryBuffer&>(in));
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
			CAGE_ASSERT(cols == png_get_rowbytes(png, info), cols, png_get_rowbytes(png, info));
			out.allocate(height * cols);
			for (uint32 y = 0; y < height; y++)
				rows[y] = (png_bytep)out.data() + y * cols;
			png_read_image(png, rows.data());
		}

		void encodePng(const MemoryBuffer &in, MemoryBuffer &out, uint32 width, uint32 height, uint32 components, uint32 bpp)
		{
			PngInfoCtx ctx;
			ctx.writing = true;
			png_structp &png = ctx.png;
			png_infop &info = ctx.info;
			png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, &pngErrFunc, nullptr);
			if (!png)
				CAGE_THROW_ERROR(Exception, "png encoder failed (png_structp)");
			PngIoCtx ioCtx(out);
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
			CAGE_ASSERT(cols == png_get_rowbytes(png, info), cols, png_get_rowbytes(png, info));
			for (uint32 y = 0; y < height; y++)
				rows[y] = (png_bytep)in.data() + y * cols;
			png_write_image(png, rows.data());
			png_write_end(png, info);
		}

		class ImageImpl : public Image
		{
		public:
			MemoryBuffer mem;
			uint32 width, height, channels, bytesPerChannel;

			ImageImpl() : width(0), height(0), channels(0), bytesPerChannel(0) {}
		};
	}

	void Image::empty(uint32 w, uint32 h, uint32 c, uint32 bpc)
	{
		ImageImpl *impl = (ImageImpl*)this;
		impl->width = w;
		impl->height = h;
		impl->channels = c;
		impl->bytesPerChannel = bpc;
		impl->mem.allocate(w * h * c * bpc);
		impl->mem.zero();
	}

	MemoryBuffer Image::encodeBuffer()
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		MemoryBuffer buffer((uintPtr)impl->width * impl->height * impl->channels * impl->bytesPerChannel);
		encodePng(impl->mem, buffer, impl->width, impl->height, impl->channels, impl->bytesPerChannel);
		return buffer;
	}

	void Image::encodeFile(const string &filename)
	{
		MemoryBuffer buffer = encodeBuffer();
		Holder<File> f = newFile(filename, FileMode(false, true));
		f->writeBuffer(buffer);
	}

	void Image::decodeMemory(const void *buffer, uintPtr size, uint32 channels, uint32 bpc)
	{
		MemoryBuffer buff(size);
		detail::memcpy(buff.data(), buffer, size);
		decodeBuffer(buff, channels, bpc);
	}

	void Image::decodeBuffer(const MemoryBuffer &buffer, uint32 channels, uint32 bpc)
	{
		ImageImpl *impl = (ImageImpl*)this;
		decodePng(buffer, impl->mem, impl->width, impl->height, impl->channels, impl->bytesPerChannel);
		if (channels == m)
			channels = impl->channels;
		if (bpc == m)
			bpc = impl->bytesPerChannel;
		convert(channels, bpc);
	}

	void Image::decodeFile(const string &filename, uint32 channels, uint32 bpc)
	{
		Holder<File> f = newFile(filename, FileMode(true, false));
		MemoryBuffer buffer(numeric_cast<uintPtr>(f->size()));
		f->read(buffer.data(), buffer.size());
		f->close();
		decodeBuffer(buffer, channels, bpc);
	}

	uint32 Image::width() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		return impl->width;
	}

	uint32 Image::height() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		return impl->height;
	}

	uint32 Image::channels() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		return impl->channels;
	}

	uint32 Image::bytesPerChannel() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		return impl->bytesPerChannel;
	}

	void *Image::bufferData()
	{
		ImageImpl *impl = (ImageImpl*)this;
		return impl->mem.data();
	}

	const void *Image::bufferData() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		return impl->mem.data();
	}

	uintPtr Image::bufferSize() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		return impl->mem.size();
	}

	float Image::value(uint32 x, uint32 y, uint32 c) const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		CAGE_ASSERT(x < impl->width && y < impl->height && c < impl->channels, x, impl->width, y, impl->height, c, impl->channels);
		switch (impl->bytesPerChannel)
		{
		case 1:
		{
			uint8 *d = (uint8*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			return *d / (float)detail::numeric_limits<uint8>::max();
		} break;
		case 2:
		{
			uint16 *d = (uint16*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			return *d / (float)detail::numeric_limits<uint16>::max();
		} break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid BPC in png image");
		}
	}

	void Image::value(uint32 x, uint32 y, uint32 c, float v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->mem.data(), "png image not initialized");
		CAGE_ASSERT(x < impl->width && y < impl->height && c < impl->channels, x, impl->width, y, impl->height, c, impl->channels);
		v = clamp(v, 0.f, 1.f);
		switch (impl->bytesPerChannel)
		{
		case 1:
		{
			uint8 *d = (uint8*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			*d = numeric_cast<uint8>(v * detail::numeric_limits<uint8>::max());
		} break;
		case 2:
		{
			uint16 *d = (uint16*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			*d = numeric_cast<uint16>(v * detail::numeric_limits<uint16>::max());
		} break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid BPC in png image");
		}
	}

	real Image::get1(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 1);
		return value(x, y, 0);
	}

	vec2 Image::get2(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 2);
		return vec2(value(x, y, 0), value(x, y, 1));
	}

	vec3 Image::get3(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 3);
		return vec3(value(x, y, 0), value(x, y, 1), value(x, y, 2));

	}

	vec4 Image::get4(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 4);
		return vec4(value(x, y, 0), value(x, y, 1), value(x, y, 2), value(x, y, 3));
	}

	void Image::set(uint32 x, uint32 y, const real &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 1);
		value(x, y, 0, v.value);
	}

	void Image::set(uint32 x, uint32 y, const vec2 &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 2);
		value(x, y, 0, v[0].value);
		value(x, y, 1, v[1].value);
	}

	void Image::set(uint32 x, uint32 y, const vec3 &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 3);
		value(x, y, 0, v[0].value);
		value(x, y, 1, v[1].value);
		value(x, y, 2, v[2].value);
	}

	void Image::set(uint32 x, uint32 y, const vec4 &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(channels() == 4);
		value(x, y, 0, v[0].value);
		value(x, y, 1, v[1].value);
		value(x, y, 2, v[2].value);
		value(x, y, 3, v[3].value);
	}

	void Image::verticalFlip()
	{
		ImageImpl *impl = (ImageImpl*)this;
		uint32 lineSize = impl->bytesPerChannel * impl->channels * impl->width;
		uint32 swapsCount = impl->height / 2;
		MemoryBuffer tmp;
		tmp.allocate(lineSize);
		for (uint32 i = 0; i < swapsCount; i++)
		{
			detail::memcpy(tmp.data(), impl->mem.data() + i * lineSize, lineSize);
			detail::memcpy(impl->mem.data() + i * lineSize, impl->mem.data() + (impl->height - i - 1) * lineSize, lineSize);
			detail::memcpy(impl->mem.data() + (impl->height - i - 1) * lineSize, tmp.data(), lineSize);
		}
	}

	void Image::convert(uint32 channels, uint32 bpc)
	{
		ImageImpl *impl = (ImageImpl*)this;
		if (impl->channels == channels && impl->bytesPerChannel == bpc)
			return;
		CAGE_THROW_CRITICAL(NotImplemented, "png convert");
	}

	Holder<Image> newImage()
	{
		return detail::systemArena().createImpl<Image, ImageImpl>();
	}

	void imageBlit(const Image *sourcePng, Image *targetPng, uint32 sourceX, uint32 sourceY, uint32 targetX, uint32 targetY, uint32 width, uint32 height)
	{
		CAGE_ASSERT(sourcePng->channels() == targetPng->channels(), "images are incompatible (different count of channels)", sourcePng->channels(), targetPng->channels());
		CAGE_ASSERT(sourcePng->bytesPerChannel() == targetPng->bytesPerChannel(), "images are incompatible (different bytes per channel)", sourcePng->bytesPerChannel(), targetPng->bytesPerChannel());
		uint32 sw = sourcePng->width();
		uint32 sh = sourcePng->height();
		uint32 tw = targetPng->width();
		uint32 th = targetPng->height();
		CAGE_ASSERT(sourceX + width <= sw);
		CAGE_ASSERT(sourceY + height <= sh);
		CAGE_ASSERT(targetX + width <= tw);
		CAGE_ASSERT(targetY + height <= th);
		uint32 bpc = sourcePng->bytesPerChannel();
		uint32 cpp = sourcePng->channels();
		uint32 ps = bpc * cpp;
		uint32 sl = sw * ps;
		uint32 tl = tw * ps;
		char *s = (char*)sourcePng->bufferData();
		char *t = (char*)targetPng->bufferData();
		for (uint32 y = 0; y < height; y++)
			detail::memcpy(t + (targetY + y) * tl + targetX * ps, s + (sourceY + y) * sl + sourceX * ps, width * ps);
	}
}
