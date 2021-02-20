#include "vorbis.h"

namespace cage
{
	class PolytoneStreamImpl : public PolytoneStream
	{
	public:
		Holder<Polytone> poly;
		Holder<VorbisDecoder> vorbis;

		PolytoneStreamImpl(Holder<Polytone> &&polytone) : poly(templates::move(polytone))
		{
			if (poly->format() == PolytoneFormatEnum::Vorbis)
				vorbis = detail::systemArena().createHolder<VorbisDecoder>(newFileBuffer(&((PolytoneImpl *)+poly)->mem));
		}
	};

	const Polytone *PolytoneStream::source() const
	{
		const PolytoneStreamImpl *impl = (const PolytoneStreamImpl *)this;
		return +impl->poly;
	}

	void PolytoneStream::decode(uint64 frameOffset, PointerRange<float> buffer)
	{
		PolytoneStreamImpl *impl = (PolytoneStreamImpl *)this;
		PolytoneImpl *src = (PolytoneImpl *)+impl->poly;
		const uint32 channels = src->channels;
		CAGE_ASSERT((buffer.size() % channels) == 0);
		const uint64 frames = buffer.size() / channels;
		switch (impl->poly->format())
		{
		case PolytoneFormatEnum::Vorbis:
		{
			impl->vorbis->seek(frameOffset);
			impl->vorbis->decode(buffer);
		} break;
		case PolytoneFormatEnum::Float:
		{
			detail::memcpy(buffer.data(), src->mem.data() + frameOffset * channels * sizeof(float), frames * channels * sizeof(float));
		} break;
		default:
		{
			for (uint64 f = 0; f < frames; f++)
				for (uint32 c = 0; c < channels; c++)
					buffer[f * channels + c] = src->value(frameOffset + f, c);
		} break;
		}
	}

	Holder<PolytoneStream> newPolytoneStream(Holder<Polytone> &&polytone)
	{
		return detail::systemArena().createImpl<PolytoneStream, PolytoneStreamImpl>(templates::move(polytone));
	}

	void vorbisConvertFormat(PolytoneImpl *snd, PolytoneFormatEnum format)
	{
		CAGE_ASSERT(snd->format != format);
		CAGE_ASSERT(snd->format == PolytoneFormatEnum::Vorbis || format == PolytoneFormatEnum::Vorbis);
		if (snd->format == PolytoneFormatEnum::Vorbis)
		{
			// vorbis -> float -> format
			MemoryBuffer tmp;
			std::swap(tmp, snd->mem);
			VorbisDecoder dec(newFileBuffer(tmp, FileMode(true, false)));
			CAGE_ASSERT(dec.frames == snd->frames);
			CAGE_ASSERT(dec.channels == snd->channels);
			snd->initialize(dec.frames, dec.channels, dec.sampleRate, PolytoneFormatEnum::Float);
			dec.decode(bufferCast<float, char>(snd->mem));
			polytoneConvertFormat(snd, format);
		}
		else
		{
			// snd->format -> float -> vorbis
			polytoneConvertFormat(snd, PolytoneFormatEnum::Float);
			VorbisEncoder enc(snd);
			enc.encode();
			std::swap(snd->mem, enc.outputBuffer);
			snd->format = PolytoneFormatEnum::Vorbis;
		}
	}

	Holder<Polytone> vorbisExtract(const PolytoneImpl *src, uint64 offset, uint64 frames)
	{
		CAGE_ASSERT(src->format == PolytoneFormatEnum::Vorbis);
		CAGE_ASSERT(offset + frames <= src->frames);
		Holder<Polytone> poly = newPolytone();
		PolytoneImpl *dst = (PolytoneImpl *)+poly;
		dst->initialize(frames, src->channels, src->sampleRate, PolytoneFormatEnum::Float);
		VorbisDecoder dec(newFileBuffer(src->mem));
		dec.seek(offset);
		dec.decode(bufferCast<float, char>(dst->mem));
		return poly;
	}
}
