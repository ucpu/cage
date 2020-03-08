#include "image.h"

#include <cage-core/files.h>

namespace cage
{
	void pngDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	void jpegDecode(const char *inBuffer, uintPtr inSize, ImageImpl *impl);
	MemoryBuffer pngEncode(ImageImpl *impl);
	MemoryBuffer jpegEncode(ImageImpl *impl);

	void Image::decodeMemory(const void *buffer, uintPtr size, uint32 channels, ImageFormatEnum format)
	{
		if (size < 8)
			CAGE_THROW_ERROR(Exception, "insufficient data to determine image format");
		static const unsigned char pngSignature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
		static const unsigned char jpegSignature[] = { 0xFF, 0xD8, 0xFF };
		if (detail::memcmp(buffer, pngSignature, sizeof(pngSignature)) == 0)
			pngDecode((char*)buffer, size, (ImageImpl*)this);
		else if (detail::memcmp(buffer, jpegSignature, sizeof(jpegSignature)) == 0)
			jpegDecode((char*)buffer, size, (ImageImpl*)this);
		else
			CAGE_THROW_ERROR(Exception, "image data do not match any known signature");
	}

	void Image::decodeBuffer(const MemoryBuffer &buffer, uint32 channels, ImageFormatEnum format)
	{
		decodeMemory(buffer.data(), buffer.size(), channels, format);
	}

	void Image::decodeFile(const string &filename, uint32 channels, ImageFormatEnum format)
	{
		Holder<File> f = newFile(filename, FileMode(true, false));
		MemoryBuffer buffer = f->readBuffer(f->size());
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
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for image encoding");
	}

	void Image::encodeFile(const string &filename)
	{
		newFile(filename, FileMode(false, true))->writeBuffer(encodeBuffer(filename));
	}
}
