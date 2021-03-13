#include "audio.h"

#include <cage-core/files.h>
#include <cage-core/string.h>

namespace cage
{
	void flacDecode(PointerRange<const char> inBuffer, AudioImpl *impl);
	void mp3Decode(PointerRange<const char> inBuffer, AudioImpl *impl);
	void oggDecode(PointerRange<const char> inBuffer, AudioImpl *impl);
	void wavDecode(PointerRange<const char> inBuffer, AudioImpl *impl);
	MemoryBuffer flacEncode(const AudioImpl *impl);
	MemoryBuffer mp3Encode(const AudioImpl *impl);
	MemoryBuffer oggEncode(const AudioImpl *impl);
	MemoryBuffer wavEncode(const AudioImpl *impl);

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

	void Audio::importBuffer(PointerRange<const char> buffer, AudioFormatEnum requestedFormat)
	{
		AudioImpl *impl = (AudioImpl *)this;
		try
		{
			impl->clear();
			if (buffer.size() < 32)
				CAGE_THROW_ERROR(Exception, "insufficient data to determine sound format");
			constexpr const uint8 wavSignature[12] = { 0x52, 0x49, 0x46, 0x46, 0,0,0,0, 0x57, 0x41, 0x56, 0x45 };
			constexpr const uint8 wavSignatureMask[12] = { 1,1,1,1, 0,0,0,0, 1,1,1,1 };
			constexpr const uint8 flacSignature[4] = { 0x66, 0x4C, 0x61, 0x43 };
			constexpr const uint8 mp3Signature[3] = { 0x49, 0x44, 0x33 };
			constexpr const uint8 oggSignature[4] = { 0x4F, 0x67, 0x67, 0x53 };
			if (copmpareWithMask(buffer, wavSignature, wavSignatureMask) == 0)
				wavDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), flacSignature, sizeof(flacSignature)) == 0)
				flacDecode(buffer, impl);
			else if (detail::memcmp(buffer.data(), mp3Signature, sizeof(mp3Signature)) == 0)
				mp3Decode(buffer, impl);
			else if (detail::memcmp(buffer.data(), oggSignature, sizeof(oggSignature)) == 0)
				oggDecode(buffer, impl);
			else
				CAGE_THROW_ERROR(Exception, "sound data do not match any known signature");
			if (requestedFormat != AudioFormatEnum::Default)
				audioConvertFormat(this, requestedFormat);
		}
		catch (...)
		{
			impl->clear();
			throw;
		}
	}

	void Audio::importFile(const string &filename, AudioFormatEnum requestedFormat)
	{
		Holder<File> f = readFile(filename);
		Holder<PointerRange<char>> buffer = f->readAll();
		f->close();
		importBuffer(buffer, requestedFormat);
	}

	Holder<PointerRange<char>> Audio::exportBuffer(const string &format) const
	{
		CAGE_ASSERT(channels() > 0);
		const string ext = toLower(pathExtractExtension(format));
		if (ext == ".wav")
			return wavEncode((const AudioImpl *)this);
		if (ext == ".flac")
			return flacEncode((const AudioImpl *)this);
		if (ext == ".mp3")
			return mp3Encode((const AudioImpl *)this);
		if (ext == ".ogg")
			return oggEncode((const AudioImpl *)this);
		CAGE_THROW_ERROR(Exception, "unrecognized file extension for sound encoding");
	}

	void Audio::exportFile(const string &filename) const
	{
		Holder<PointerRange<char>> buf = exportBuffer(filename);
		writeFile(filename)->write(buf);
	}
}
