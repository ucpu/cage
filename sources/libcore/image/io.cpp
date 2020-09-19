#include "image.h"

#include <cage-core/files.h>
#include <cage-core/string.h>

namespace cage
{
	void pngDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void jpegDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void tiffDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void tgaDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void psdDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void ddsDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	MemoryBuffer pngEncode(ImageImpl *impl);
	MemoryBuffer jpegEncode(ImageImpl *impl);
	MemoryBuffer tiffEncode(ImageImpl *impl);
	MemoryBuffer tgaEncode(ImageImpl *impl);
	MemoryBuffer psdEncode(ImageImpl *impl);
	MemoryBuffer ddsEncode(ImageImpl *impl);

	void Image::importBuffer(PointerRange<const char> buffer, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl*)this;
		try
		{
			impl->clear();
			if (buffer.size() < 32)
				CAGE_THROW_ERROR(Exception, "insufficient data to determine image format");
			static constexpr uint8 pngSignature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
			static constexpr uint8 jpegSignature[3] = { 0xFF, 0xD8, 0xFF };
			static constexpr uint8 tiffSignature[4] = { 0x49, 0x49, 0x2A, 0x00 };
			static constexpr uint8 tgaSignature[18] = "TRUEVISION-XFILE.";
			static constexpr uint8 psdSignature[4] = { '8', 'B', 'P', 'S' };
			static constexpr uint8 ddsSignature[4] = { 'D', 'D', 'S', ' ' };
			if (detail::memcmp(buffer.data(), pngSignature, sizeof(pngSignature)) == 0)
				pngDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), jpegSignature, sizeof(jpegSignature)) == 0)
				jpegDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), tiffSignature, sizeof(tiffSignature)) == 0)
				tiffDecode(buffer, impl);
			else if (detail::memcmp(buffer.end() - sizeof(tgaSignature), tgaSignature, sizeof(tgaSignature)) == 0
				|| detail::memcmp(buffer.end() - sizeof(tgaSignature) + 1, tgaSignature, sizeof(tgaSignature) - 1) == 0)
				tgaDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), psdSignature, sizeof(psdSignature)) == 0)
				psdDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), ddsSignature, sizeof(ddsSignature)) == 0)
				ddsDecode(buffer, impl);
			else
				CAGE_THROW_ERROR(Exception, "image data do not match any known signature");
			if (channels != m)
				imageConvert(this, channels);
			if (format != ImageFormatEnum::Default)
				imageConvert(this, format);
		}
		catch (...)
		{
			impl->clear();
			throw;
		}
	}

	void Image::importFile(const string &filename, uint32 channels, ImageFormatEnum format)
	{
		Holder<File> f = newFile(filename, FileMode(true, false));
		MemoryBuffer buffer = f->read(f->size());
		f->close();
		importBuffer(buffer, channels, format);
	}

	MemoryBuffer Image::exportBuffer(const string &format) const
	{
		CAGE_ASSERT(channels() > 0);
		string ext = toLower(pathExtractExtension(format));
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
		if (ext == ".dds")
			return ddsEncode((ImageImpl *)this);
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for image encoding");
	}

	void Image::exportFile(const string &filename) const
	{
		MemoryBuffer buf = exportBuffer(filename);
		writeFile(filename)->write(buf);
	}
}
