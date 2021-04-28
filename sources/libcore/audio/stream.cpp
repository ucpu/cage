#include "vorbis.h"

namespace cage
{
	class AudioStreamImpl : public AudioStream
	{
	public:
		Holder<Audio> poly;
		Holder<VorbisDecoder> vorbis;

		AudioStreamImpl(Holder<Audio> &&audio) : poly(templates::move(audio))
		{
			if (poly->format() == AudioFormatEnum::Vorbis)
				vorbis = systemArena().createHolder<VorbisDecoder>(newFileBuffer(&((AudioImpl *)+poly)->mem));
		}
	};

	const Audio *AudioStream::source() const
	{
		const AudioStreamImpl *impl = (const AudioStreamImpl *)this;
		return +impl->poly;
	}

	void AudioStream::decode(uintPtr frameOffset, PointerRange<float> buffer)
	{
		AudioStreamImpl *impl = (AudioStreamImpl *)this;
		AudioImpl *src = (AudioImpl *)+impl->poly;
		const uint32 channels = src->channels;
		CAGE_ASSERT((buffer.size() % channels) == 0);
		const uintPtr frames = buffer.size() / channels;
		switch (impl->poly->format())
		{
		case AudioFormatEnum::Vorbis:
		{
			impl->vorbis->seek(frameOffset);
			impl->vorbis->decode(buffer);
		} break;
		case AudioFormatEnum::Float:
		{
			detail::memcpy(buffer.data(), src->mem.data() + frameOffset * channels * sizeof(float), frames * channels * sizeof(float));
		} break;
		default:
		{
			for (uintPtr f = 0; f < frames; f++)
				for (uint32 c = 0; c < channels; c++)
					buffer[f * channels + c] = src->value(frameOffset + f, c);
		} break;
		}
	}

	Holder<AudioStream> newAudioStream(Holder<Audio> &&audio)
	{
		return systemArena().createImpl<AudioStream, AudioStreamImpl>(templates::move(audio));
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
			VorbisDecoder dec(newFileBuffer(tmp, FileMode(true, false)));
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
		VorbisDecoder dec(newFileBuffer(src->mem));
		dec.seek(offset);
		dec.decode(bufferCast<float, char>(dst->mem));
		return poly;
	}
}
