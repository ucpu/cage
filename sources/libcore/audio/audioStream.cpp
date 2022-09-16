#include "vorbis.h"

#include <cage-core/audioAlgorithms.h>

namespace cage
{
	void Audio::decode(uintPtr startFrame, PointerRange<float> buffer) const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		const uint32 channels = impl->channels;
		CAGE_ASSERT((buffer.size() % channels) == 0);
		const uintPtr frames = buffer.size() / channels;
		switch (impl->format)
		{
		case AudioFormatEnum::Vorbis:
		{
			VorbisDecoder dec(newFileBuffer(Holder<const MemoryBuffer>(&impl->mem, nullptr)));
			dec.seek(startFrame);
			dec.decode(buffer);
		} break;
		case AudioFormatEnum::Float:
		{
			detail::memcpy(buffer.data(), impl->mem.data() + startFrame * channels * sizeof(float), frames * channels * sizeof(float));
		} break;
		default:
		{
			for (uintPtr f = 0; f < frames; f++)
				for (uint32 c = 0; c < channels; c++)
					buffer[f * channels + c] = impl->value(startFrame + f, c);
		} break;
		}
	}

	void vorbisConvertFormat(AudioImpl *snd, AudioFormatEnum format)
	{
		CAGE_ASSERT(snd->format != format);
		CAGE_ASSERT(snd->format == AudioFormatEnum::Vorbis || format == AudioFormatEnum::Vorbis);
		if (snd->format == AudioFormatEnum::Vorbis)
		{
			// vorbis -> float -> format
			MemoryBuffer tmp;
			std::swap(tmp, snd->mem);
			VorbisDecoder dec(newFileBuffer(Holder<MemoryBuffer>(&tmp, nullptr), FileMode(true, false)));
			CAGE_ASSERT(dec.frames == snd->frames);
			CAGE_ASSERT(dec.channels == snd->channels);
			snd->initialize(dec.frames, dec.channels, dec.sampleRate, AudioFormatEnum::Float);
			dec.decode(bufferCast<float, char>(snd->mem));
			audioConvertFormat(snd, format);
		}
		else
		{
			// snd->format -> float -> vorbis
			audioConvertFormat(snd, AudioFormatEnum::Float);
			VorbisEncoder enc(snd);
			enc.encode();
			std::swap(snd->mem, enc.outputBuffer);
			snd->format = AudioFormatEnum::Vorbis;
		}
	}

	Holder<Audio> vorbisExtract(const AudioImpl *src, uintPtr offset, uintPtr frames)
	{
		CAGE_ASSERT(src->format == AudioFormatEnum::Vorbis);
		CAGE_ASSERT(offset + frames <= src->frames);
		Holder<Audio> poly = newAudio();
		AudioImpl *dst = (AudioImpl *)+poly;
		dst->initialize(frames, src->channels, src->sampleRate, AudioFormatEnum::Float);
		VorbisDecoder dec(newFileBuffer(Holder<const MemoryBuffer>(&src->mem, nullptr)));
		dec.seek(offset);
		dec.decode(bufferCast<float, char>(dst->mem));
		return poly;
	}
}
