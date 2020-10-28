#include "image.h"
#include <cage-core/stdBufferStream.h>

#include <tiffio.h>
#include <tiffio.hxx>

namespace cage
{
	namespace
	{
#define GCHL_LOG(SEVERITY) { char buffer[1000]; vsprintf(buffer, fmt, args); CAGE_LOG(SEVERITY, "tiff", buffer); }

		void tiffErrorHandler(const char *modul, const char *fmt, va_list args)
		{
			GCHL_LOG(SeverityEnum::Error);
		}

		void tiffWarningHandler(const char *modul, const char *fmt, va_list args)
		{
			GCHL_LOG(SeverityEnum::Warning);
		}

		class Initializer
		{
		public:
			Initializer()
			{
				TIFFSetErrorHandler(&tiffErrorHandler);
				TIFFSetWarningHandler(&tiffWarningHandler);
			}
		} initializer;
	}

	void tiffDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		TIFF *t = nullptr;
		try
		{
			BufferIStream stream(inBuffer);
			t = TIFFStreamOpen("MemTIFF", &stream);
			if (!t)
				CAGE_THROW_ERROR(Exception, "failed to initialize tiff decoding");
			{
				uint16 planarconfig = 0;
				TIFFGetField(t, TIFFTAG_PLANARCONFIG, &planarconfig);
				if (planarconfig != PLANARCONFIG_CONTIG)
					CAGE_THROW_ERROR(Exception, "unsupported planar configuration in tiff decoding");
			}
			TIFFGetField(t, TIFFTAG_IMAGEWIDTH, &impl->width);
			TIFFGetField(t, TIFFTAG_IMAGELENGTH, &impl->height);
			{
				uint16 ch = 0;
				TIFFGetField(t, TIFFTAG_SAMPLESPERPIXEL, &ch);
				impl->channels = ch;
			}
			{
				uint16 bpp = 0;
				TIFFGetField(t, TIFFTAG_BITSPERSAMPLE, &bpp);
				uint16 sampleformat = 0;
				TIFFGetField(t, TIFFTAG_SAMPLEFORMAT, &sampleformat);
				if (sampleformat == 0)
					sampleformat = SAMPLEFORMAT_UINT;
				if (bpp == 8 && sampleformat == SAMPLEFORMAT_UINT)
					impl->format = ImageFormatEnum::U8;
				else if (bpp == 16 && sampleformat == SAMPLEFORMAT_UINT)
					impl->format = ImageFormatEnum::U16;
				else if (bpp == 32 && sampleformat == SAMPLEFORMAT_IEEEFP)
					impl->format = ImageFormatEnum::Float;
				else
					CAGE_THROW_ERROR(Exception, "unsupported format in tiff decoding");
			}
			uint32 stride = numeric_cast<uint32>(TIFFScanlineSize(t));
			CAGE_ASSERT(stride == impl->width * impl->channels * formatBytes(impl->format));
			impl->mem.resize(impl->height * stride);
			for (uint32 row = 0; row < impl->height; row++)
			{
				char *dst = impl->mem.data() + row * stride;
				if (TIFFReadScanline(t, dst, row) < 0)
					CAGE_THROW_ERROR(Exception, "failed scanline reading in tiff decoding");
			}
			TIFFClose(t);

			// color config
			// todo deduce it from the file
		}
		catch (...)
		{
			if (t)
				TIFFClose(t);
			throw;
		}
	}

	MemoryBuffer tiffEncode(const ImageImpl *impl)
	{
		TIFF *t = nullptr;
		try
		{
			MemoryBuffer res;
			res.reserve(impl->width * impl->height * impl->channels * formatBytes(impl->format) + 100);
			BufferOStream stream(res);
			t = TIFFStreamOpen("MemTIFF", &stream);
			if (!t)
				CAGE_THROW_ERROR(Exception, "failed to initialize tiff encoding");
			TIFFSetField(t, TIFFTAG_IMAGEWIDTH, impl->width);
			TIFFSetField(t, TIFFTAG_IMAGELENGTH, impl->height);
			TIFFSetField(t, TIFFTAG_SAMPLESPERPIXEL, impl->channels);
			switch (impl->format)
			{
			case ImageFormatEnum::U8:
				TIFFSetField(t, TIFFTAG_BITSPERSAMPLE, 8);
				TIFFSetField(t, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
				break;
			case ImageFormatEnum::U16:
				TIFFSetField(t, TIFFTAG_BITSPERSAMPLE, 16);
				TIFFSetField(t, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
				break;
			case ImageFormatEnum::Float:
				TIFFSetField(t, TIFFTAG_BITSPERSAMPLE, 32);
				TIFFSetField(t, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
				break;
			default:
				CAGE_THROW_ERROR(Exception, "unsupported format in tiff encoding");
			}
			TIFFSetField(t, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
			TIFFSetField(t, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
			switch (impl->channels)
			{
			case 1:
			case 2:
				TIFFSetField(t, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
				break;
			default:
				TIFFSetField(t, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
				break;
			}
			TIFFSetField(t, TIFFTAG_SOFTWARE, "CageEngine");
			uint32 stride = numeric_cast<uint32>(TIFFScanlineSize(t));
			CAGE_ASSERT(stride == impl->width * impl->channels * formatBytes(impl->format));
			for (uint32 row = 0; row < impl->height; row++)
			{
				const char *src = impl->mem.data() + row * stride;
				if (TIFFWriteScanline(t, (void *)src, row) < 0)
					CAGE_THROW_ERROR(Exception, "failed scanline writing in tiff encoding");
			}
			TIFFClose(t);
			return res;
		}
		catch (...)
		{
			if (t)
				TIFFClose(t);
			throw;
		}
	}
}
