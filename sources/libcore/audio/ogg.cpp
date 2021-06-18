#include "vorbis.h"

namespace cage
{
	void oggDecode(PointerRange<const char> inBuffer, AudioImpl *impl)
	{
		VorbisDecoder dec(newFileBuffer(Holder<PointerRange<const char>>(&inBuffer, nullptr)));
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
		else if (impl->format == AudioFormatEnum::Float)
		{
			VorbisEncoder enc(impl);
			enc.encode();
			return std::move(enc.outputBuffer);
		}
		else
		{
			Holder<Audio> a = impl->copy();
			audioConvertFormat(+a, AudioFormatEnum::Float);
			VorbisEncoder enc((AudioImpl *)+a);
			enc.encode();
			return std::move(enc.outputBuffer);
		}
	}
}
