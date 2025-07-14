#include "image.h"

#include <cage-core/files.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/imageImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>

namespace cage
{
	void pngDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void jpegDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void tiffDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void psdDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void ddsDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void exrDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	void tgaDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	bool experimentalTgaDecode(PointerRange<const char> inBuffer, ImageImpl *impl);
	ImageImportResult ddsDecode(PointerRange<const char> inBuffer);

	MemoryBuffer pngEncode(const ImageImpl *impl);
	MemoryBuffer jpegEncode(const ImageImpl *impl);
	MemoryBuffer tiffEncode(const ImageImpl *impl);
	MemoryBuffer psdEncode(const ImageImpl *impl);
	MemoryBuffer ddsEncode(const ImageImpl *impl);
	MemoryBuffer exrEncode(const ImageImpl *impl);
	MemoryBuffer tgaEncode(const ImageImpl *impl);

	namespace
	{
		constexpr const uint8 pngSignature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
		constexpr const uint8 jpegSignature[3] = { 0xFF, 0xD8, 0xFF };
		constexpr const uint8 tiffSignature[4] = { 0x49, 0x49, 0x2A, 0x00 };
		constexpr const uint8 psdSignature[4] = { '8', 'B', 'P', 'S' };
		constexpr const uint8 ddsSignature[4] = { 'D', 'D', 'S', ' ' };
		constexpr const uint8 exrSignature[4] = { 0x76, 0x2F, 0x31, 0x01 };
		constexpr const uint8 tgaSignature1[18] = "TRUEVISION-XFILE.";
		constexpr const uint8 tgaSignature2[17] = { 'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.' }; // no terminal zero

		template<uint32 N>
		bool compare(PointerRange<const char> buffer, const uint8 (&signature)[N])
		{
			return detail::memcmp(buffer.data(), signature, N) == 0;
		}

		PointerRange<const char> buffEnd(PointerRange<const char> buffer, uint32 size)
		{
			return { buffer.end() - size, buffer.end() };
		}
	}

	void Image::importBuffer(PointerRange<const char> buffer, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl *)this;
		impl->clear();
		try
		{
			if (buffer.size() < 32)
				CAGE_THROW_ERROR(Exception, "insufficient data to determine image format");

			if (compare(buffer, pngSignature))
				pngDecode(buffer, impl);
			else if (compare(buffer, jpegSignature))
				jpegDecode(buffer, impl);
			else if (compare(buffer, tiffSignature))
				tiffDecode(buffer, impl);
			else if (compare(buffer, psdSignature))
				psdDecode(buffer, impl);
			else if (compare(buffer, ddsSignature))
				ddsDecode(buffer, impl);
			else if (compare(buffer, exrSignature))
				exrDecode(buffer, impl);
			else if (compare(buffEnd(buffer, sizeof(tgaSignature1)), tgaSignature1) || compare(buffEnd(buffer, sizeof(tgaSignature2)), tgaSignature2))
				tgaDecode(buffer, impl);
			else if (experimentalTgaDecode(buffer, impl))
				CAGE_LOG(SeverityEnum::Warning, "cage", "decoded image with EXPERIMENTAL tga codec");
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

	void Image::importFile(const String &filename, uint32 channels, ImageFormatEnum format)
	{
		Holder<File> f = readFile(filename);
		Holder<PointerRange<char>> buffer = f->readAll();
		f->close();
		importBuffer(buffer, channels, format);
	}

	Holder<PointerRange<char>> Image::exportBuffer(const String &format) const
	{
		if (channels() == 0)
			CAGE_THROW_ERROR(Exception, "cannot export invalid image");
		const String ext = toLower(pathExtractExtension(format));
		if (ext == ".png")
			return pngEncode((const ImageImpl *)this);
		if (ext == ".jpeg" || ext == ".jpg")
			return jpegEncode((const ImageImpl *)this);
		if (ext == ".tiff" || ext == ".tif")
			return tiffEncode((const ImageImpl *)this);
		if (ext == ".psd" || ext == ".psb")
			return psdEncode((const ImageImpl *)this);
		if (ext == ".dds")
			return ddsEncode((const ImageImpl *)this);
		if (ext == ".exr")
			return exrEncode((const ImageImpl *)this);
		if (ext == ".tga")
			return tgaEncode((const ImageImpl *)this);
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for image encoding");
	}

	void Image::exportFile(const String &filename) const
	{
		Holder<PointerRange<char>> buf = exportBuffer(filename);
		writeFile(filename)->write(buf);
	}

	namespace
	{
		ImageImportResult decodeSingle(PointerRange<const char> buffer)
		{
			Holder<Image> img = newImage();
			img->importBuffer(buffer);
			ImageImportPart part;
			part.image = std::move(img);
			PointerRangeHolder<ImageImportPart> parts;
			parts.push_back(std::move(part));
			ImageImportResult res;
			res.parts = std::move(parts);
			return res;
		}
	}

	ImageImportResult imageImportBuffer(PointerRange<const char> buffer)
	{
		if (buffer.size() < 32)
			CAGE_THROW_ERROR(Exception, "insufficient data to determine image format");
		// todo tiff
		if (compare(buffer, ddsSignature))
			return ddsDecode(buffer);
		return decodeSingle(buffer);
	}
}
