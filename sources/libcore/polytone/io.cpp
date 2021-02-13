#include "polytone.h"

#include <cage-core/files.h>
#include <cage-core/string.h>

namespace cage
{
	void flacDecode(PointerRange<const char> inBuffer, PolytoneImpl *impl);
	void mp3Decode(PointerRange<const char> inBuffer, PolytoneImpl *impl);
	void vorbisDecode(PointerRange<const char> inBuffer, PolytoneImpl *impl);
	void wavDecode(PointerRange<const char> inBuffer, PolytoneImpl *impl);
	MemoryBuffer flacEncode(const PolytoneImpl *impl);
	MemoryBuffer mp3Encode(const PolytoneImpl *impl);
	MemoryBuffer vorbisEncode(const PolytoneImpl *impl);
	MemoryBuffer wavEncode(const PolytoneImpl *impl);

	namespace
	{
		int copmpareWithMask(PointerRange<const char> buffer, PointerRange<const uint8> signature, PointerRange<const uint8> mask)
		{
			CAGE_ASSERT(buffer.size() >= signature.size());
			CAGE_ASSERT(mask.size() == signature.size());
			for (uint32 i = 0; i < signature.size(); i++)
			{
				if (buffer[i] * mask[i] != signature[i])
					return -1;
			}
			return 0;
		}
	}

	void Polytone::importBuffer(PointerRange<const char> buffer, PolytoneFormatEnum requestedFormat)
	{
		PolytoneImpl *impl = (PolytoneImpl *)this;
		try
		{
			impl->clear();
			if (buffer.size() < 32)
				CAGE_THROW_ERROR(Exception, "insufficient data to determine sound format");
			constexpr const uint8 flacSignature[4] = { 0x66, 0x4C, 0x61, 0x43 };
			constexpr const uint8 mp3Signature[3] = { 0x49, 0x44, 0x33 };
			constexpr const uint8 vorbisSignature[4] = { 0x4F, 0x67, 0x67, 0x53 };
			constexpr const uint8 wavSignature[12] = { 0x52, 0x49, 0x46, 0x46, 0,0,0,0, 0x57, 0x41, 0x56, 0x45 };
			constexpr const uint8 wavSignatureMask[12] = { 1,1,1,1, 0,0,0,0, 1,1,1,1 };
			if (detail::memcmp(buffer.data(), flacSignature, sizeof(flacSignature)) == 0)
				flacDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), mp3Signature, sizeof(mp3Signature)) == 0)
				mp3Decode(buffer, impl);
			else if (detail::memcmp(buffer.data(), vorbisSignature, sizeof(vorbisSignature)) == 0)
				vorbisDecode(buffer, impl);
			else if (copmpareWithMask(buffer, wavSignature, wavSignatureMask) == 0)
				wavDecode(buffer, impl);
			else
				CAGE_THROW_ERROR(Exception, "sound data do not match any known signature");
			if (requestedFormat != PolytoneFormatEnum::Default)
				polytoneConvertFormat(this, requestedFormat);
		}
		catch (...)
		{
			impl->clear();
			throw;
		}
	}

	void Polytone::importFile(const string &filename, PolytoneFormatEnum requestedFormat)
	{
		Holder<File> f = readFile(filename);
		Holder<PointerRange<char>> buffer = f->readAll();
		f->close();
		importBuffer(buffer, requestedFormat);
	}

	Holder<PointerRange<char>> Polytone::exportBuffer(const string &format) const
	{
		CAGE_ASSERT(channels() > 0);
		const string ext = toLower(pathExtractExtension(format));
		if (ext == ".flac")
			return flacEncode((const PolytoneImpl *)this);
		if (ext == ".mp3")
			return mp3Encode((const PolytoneImpl *)this);
		if (ext == ".ogg")
			return vorbisEncode((const PolytoneImpl *)this);
		if (ext == ".wav")
			return wavEncode((const PolytoneImpl *)this);
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for sound encoding");
	}

	void Polytone::exportFile(const string &filename) const
	{
		Holder<PointerRange<char>> buf = exportBuffer(filename);
		writeFile(filename)->write(buf);
	}
}
