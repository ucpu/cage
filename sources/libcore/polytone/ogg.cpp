#include "vorbis.h"

namespace cage
{
	void oggDecode(PointerRange<const char> inBuffer, PolytoneImpl *impl)
	{
		VorbisDecoder dec(newFileBuffer(inBuffer));
		impl->mem.resize(0); // avoid copying
		impl->mem.resize(inBuffer.size());
		detail::memcpy(impl->mem.data(), inBuffer.data(), inBuffer.size());
		impl->frames = dec.frames;
		impl->channels = dec.channels;
		impl->sampleRate = dec.sampleRate;
		impl->format = PolytoneFormatEnum::Vorbis;
	}

	MemoryBuffer oggEncode(const PolytoneImpl *impl)
	{
		if (impl->format == PolytoneFormatEnum::Vorbis)
			return impl->mem.copy();
		else
		{
			VorbisEncoder enc(impl);
			enc.encode();
			return templates::move(enc.outputBuffer);
		}
	}
}
