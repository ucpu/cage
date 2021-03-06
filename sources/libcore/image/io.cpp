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
	void exrDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	MemoryBuffer pngEncode(const ImageImpl *impl);
	MemoryBuffer jpegEncode(const ImageImpl *impl);
	MemoryBuffer tiffEncode(const ImageImpl *impl);
	MemoryBuffer tgaEncode(const ImageImpl *impl);
	MemoryBuffer psdEncode(const ImageImpl *impl);
	MemoryBuffer ddsEncode(const ImageImpl *impl);
	MemoryBuffer exrEncode(const ImageImpl *impl);

	void Image::importBuffer(PointerRange<const char> buffer, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl*)this;
		try
		{
			impl->clear();
			if (buffer.size() < 32)
				CAGE_THROW_ERROR(Exception, "insufficient data to determine image format");
			constexpr const uint8 pngSignature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
			constexpr const uint8 jpegSignature[3] = { 0xFF, 0xD8, 0xFF };
			constexpr const uint8 tiffSignature[4] = { 0x49, 0x49, 0x2A, 0x00 };
			constexpr const uint8 tgaSignature[18] = "TRUEVISION-XFILE.";
			constexpr const uint8 psdSignature[4] = { '8', 'B', 'P', 'S' };
			constexpr const uint8 ddsSignature[4] = { 'D', 'D', 'S', ' ' };
			constexpr const uint8 exrSignature[4] = { 0x76, 0x2F, 0x31, 0x01 };
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
			else if (detail::memcmp(buffer.data(), exrSignature, sizeof(exrSignature)) == 0)
			{
				if (format != ImageFormatEnum::Default && format != ImageFormatEnum::Float)
					CAGE_THROW_ERROR(Exception, "exr image must be loaded with float format");
				exrDecode(buffer, impl);
			}
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
		Holder<File> f = readFile(filename);
		Holder<PointerRange<char>> buffer = f->readAll();
		f->close();
		importBuffer(buffer, channels, format);
	}

	Holder<PointerRange<char>> Image::exportBuffer(const string &format) const
	{
		CAGE_ASSERT(channels() > 0);
		const string ext = toLower(pathExtractExtension(format));
		if (ext == ".png")
			return pngEncode((const ImageImpl *)this);
		if (ext == ".jpeg" || ext == ".jpg")
			return jpegEncode((const ImageImpl *)this);
		if (ext == ".tiff" || ext == ".tif")
			return tiffEncode((const ImageImpl *)this);
		if (ext == ".tga")
			return tgaEncode((const ImageImpl *)this);
		if (ext == ".psd" || ext == ".psb")
			return psdEncode((const ImageImpl *)this);
		if (ext == ".dds")
			return ddsEncode((const ImageImpl *)this);
		if (ext == ".exr")
			return exrEncode((const ImageImpl *)this);
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for image encoding");
	}

	void Image::exportFile(const string &filename) const
	{
		Holder<PointerRange<char>> buf = exportBuffer(filename);
		writeFile(filename)->write(buf);
	}
}
