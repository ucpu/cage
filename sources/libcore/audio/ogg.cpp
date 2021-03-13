#include "vorbis.h"

namespace cage
{
	void oggDecode(PointerRange<const char> inBuffer, AudioImpl *impl)
	{
		VorbisDecoder dec(newFileBuffer(inBuffer));
		impl->mem.resize(0); // avoid copying
		impl->mem.resize(inBuffer.size());
		detail::memcpy(impl->mem.data(), inBuffer.data(), inBuffer.size());
		impl->frames = dec.frames;
		impl->channels = dec.channels;
		impl->sampleRate = dec.sampleRate;
		impl->format = AudioFormatEnum::Vorbis;
	}

	MemoryBuffer oggEncode(const AudioImpl *impl)
	{
		if (impl->format == AudioFormatEnum::Vorbis)
			return impl->mem.copy();
		else
		{
			VorbisEncoder enc(impl);
			enc.encode();
			return templates::move(enc.outputBuffer);
		}
	}
}
