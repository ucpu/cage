#include "image.h"

#include <cage-core/files.h>

namespace cage
{
	void pngDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	void jpegDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	void tiffDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	void tgaDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	void psdDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	MemoryBuffer pngEncode(ImageImpl *impl);
	MemoryBuffer jpegEncode(ImageImpl *impl);
	MemoryBuffer tiffEncode(ImageImpl *impl);
	MemoryBuffer tgaEncode(ImageImpl *impl);
	MemoryBuffer psdEncode(ImageImpl *impl);

	void Image::decodeMemory(const void *buffer, uintPtr size, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl*)this;
		try
		{
			impl->reset();
			if (size < 32)
				CAGE_THROW_ERROR(Exception, "insufficient data to determine image format");
			static const unsigned char pngSignature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
			static const unsigned char jpegSignature[3] = { 0xFF, 0xD8, 0xFF };
			static const unsigned char tiffSignature[4] = { 0x49, 0x49, 0x2A, 0x00 };
			static const unsigned char tgaSignature[18] = "TRUEVISION-XFILE.";
			static const unsigned char psdSignature[4] = { '8', 'B', 'P', 'S' };
			if (detail::memcmp(buffer, pngSignature, sizeof(pngSignature)) == 0)
				pngDecode((char*)buffer, size, impl);
			else if (detail::memcmp(buffer, jpegSignature, sizeof(jpegSignature)) == 0)
				jpegDecode((char*)buffer, size, impl);
			else if (detail::memcmp(buffer, tiffSignature, sizeof(tiffSignature)) == 0)
				tiffDecode((char*)buffer, size, impl);
			else if (detail::memcmp((char*)buffer + size - sizeof(tgaSignature), tgaSignature, sizeof(tgaSignature)) == 0)
				tgaDecode((char*)buffer, size, impl);
			else if (detail::memcmp(buffer, psdSignature, sizeof(psdSignature)) == 0)
				psdDecode((char*)buffer, size, impl);
			else
				CAGE_THROW_ERROR(Exception, "image data do not match any known signature");
			if (channels != m)
				convert(channels);
			if (format != ImageFormatEnum::Default)
				convert(format);
		}
		catch (...)
		{
			impl->reset();
			throw;
		}
	}

	void Image::decodeBuffer(const MemoryBuffer &buffer, uint32 channels, ImageFormatEnum format)
	{
		decodeMemory(buffer.data(), buffer.size(), channels, format);
	}

	void Image::decodeFile(const string &filename, uint32 channels, ImageFormatEnum format)
	{
		Holder<File> f = newFile(filename, FileMode(true, false));
		MemoryBuffer buffer = f->readBuffer(f->size());
		f->close();
		decodeBuffer(buffer, channels, format);
	}

	MemoryBuffer Image::encodeBuffer(const string &format)
	{
		CAGE_ASSERT(channels() > 0);
		string ext = pathExtractExtension(format).toLower();
		if (ext == ".png")
			return pngEncode((ImageImpl *)this);
		if (ext == ".jpeg" || ext == ".jpg")
			return jpegEncode((ImageImpl *)this);
		if (ext == ".tiff" || ext == ".tif")
			return tiffEncode((ImageImpl *)this);
		if (ext == ".tga")
			return tgaEncode((ImageImpl *)this);
		if (ext == ".psd" || ext == ".psb")
			return psdEncode((ImageImpl *)this);
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for image encoding");
	}

	void Image::encodeFile(const string &filename)
	{
		MemoryBuffer buf = encodeBuffer(filename);
		newFile(filename, FileMode(false, true))->writeBuffer(buf);
	}
}
