#include <cage-core/math.h>
#include <cage-core/serialization.h>
#include <cage-core/sampleRateConverter.h>

#include "audio.h"

#include <utility> // std::swap

namespace cage
{
	uint32 formatBytes(AudioFormatEnum format)
	{
		switch (format)
		{
		case AudioFormatEnum::S16: return sizeof(sint16);
		case AudioFormatEnum::S32: return sizeof(sint32);
		case AudioFormatEnum::Float: return sizeof(float);
		case AudioFormatEnum::Vorbis:
			CAGE_THROW_ERROR(Exception, "vorbis encoding has variable bitrate");
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid audio format");
		}
	}

	void Audio::clear()
	{
		AudioImpl *impl = (AudioImpl *)this;
		impl->frames = 0;
		impl->channels = 0;
		impl->sampleRate = 0;
		impl->format = AudioFormatEnum::Default;
		impl->mem.resize(0);
	}

	void Audio::initialize(uint64 frames, uint32 channels, uint32 sampleRate, AudioFormatEnum format)
	{
		CAGE_ASSERT(format != AudioFormatEnum::Vorbis);
		CAGE_ASSERT(format != AudioFormatEnum::Default);
		CAGE_ASSERT(channels > 0);
		AudioImpl *impl = (AudioImpl *)this;
		impl->frames = frames;
		impl->channels = channels;
		impl->sampleRate = sampleRate;
		impl->format = format;
		impl->mem.resize(0); // avoid unnecessary copies without deallocating the memory
		impl->mem.resize(frames * channels * formatBytes(format));
		impl->mem.zero();
	}

	Holder<Audio> Audio::copy() const
	{
		const AudioImpl *src = (const AudioImpl *)this;
		Holder<Audio> poly = newAudio();
		AudioImpl *dst = (AudioImpl *)+poly;
		dst->mem = src->mem.copy();
		dst->frames = src->frames;
		dst->channels = src->channels;
		dst->sampleRate = src->sampleRate;
		dst->format = src->format;
		return poly;
	}

	void Audio::importRaw(PointerRange<const char> buffer, uint64 frames, uint32 channels, uint32 sampleRate, AudioFormatEnum format)
	{
		AudioImpl *impl = (AudioImpl *)this;
		CAGE_ASSERT(buffer.size() >= frames * channels * formatBytes(format));
		initialize(frames, channels, sampleRate, format);
		detail::memcpy(impl->mem.data(), buffer.data(), impl->mem.size());
	}

	PointerRange<const sint16> Audio::rawViewS16() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		CAGE_ASSERT(impl->format == AudioFormatEnum::S16);
		return bufferCast<const sint16, const char>(impl->mem);
	}

	PointerRange<const sint32> Audio::rawViewS32() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		CAGE_ASSERT(impl->format == AudioFormatEnum::S32);
		return bufferCast<const sint32, const char>(impl->mem);
	}

	PointerRange<const float> Audio::rawViewFloat() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		CAGE_ASSERT(impl->format == AudioFormatEnum::Float);
		return bufferCast<const float, const char>(impl->mem);
	}

	PointerRange<const char> Audio::rawViewVorbis() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		CAGE_ASSERT(impl->format == AudioFormatEnum::Vorbis);
		return impl->mem;
	}

	uint64 Audio::frames() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		return impl->frames;
	}

	uint32 Audio::channels() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		return impl->channels;
	}

	uint32 Audio::sampleRate() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		return impl->sampleRate;
	}

	AudioFormatEnum Audio::format() const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		return impl->format;
	}

	uint64 Audio::duration() const
	{
		return 1000000 * frames() / sampleRate();
	}

	float Audio::value(uint64 f, uint32 c) const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		CAGE_ASSERT(f < impl->frames && c < impl->channels);
		const uint64 offset = f * impl->channels + c;
		switch (impl->format)
		{
		case AudioFormatEnum::S16:
			return ((sint16 *)impl->mem.data())[offset] / 32767.f;
		case AudioFormatEnum::S32:
			return ((sint32 *)impl->mem.data())[offset] / 2147483647.f;
		case AudioFormatEnum::Float:
			return ((float *)impl->mem.data())[offset];
		case AudioFormatEnum::Vorbis:
			CAGE_THROW_ERROR(Exception, "cannot directly sample vorbis encoded audio");
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid audio format");
		}
	}

	void Audio::value(uint64 f, uint32 c, float v)
	{
		AudioImpl *impl = (AudioImpl *)this;
		CAGE_ASSERT(f < impl->frames &&c < impl->channels);
		const uint64 offset = f * impl->channels + c;
		switch (impl->format)
		{
		case AudioFormatEnum::S16:
			((sint16 *)impl->mem.data())[offset] = numeric_cast<sint16>(clamp(real(v), -1, 1) * 32767.f);
			break;
		case AudioFormatEnum::S32:
			((sint32 *)impl->mem.data())[offset] = numeric_cast<sint32>(clamp(real(v), -1, 1) * 2147483647.f);
			break;
		case AudioFormatEnum::Float:
			((float *)impl->mem.data())[offset] = v;
			break;
		case AudioFormatEnum::Vorbis:
			CAGE_THROW_ERROR(Exception, "cannot directly sample vorbis encoded audio");
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid audio format");
		}
	}

	void Audio::value(uint64 f, uint32 c, const real &v) { value(f, c, v.value); }

	Holder<Audio> newAudio()
	{
		return systemArena().createImpl<Audio, AudioImpl>();
	}

	void audioSetSampleRate(Audio *snd, uint32 sampleRate)
	{
		AudioImpl *impl = (AudioImpl *)snd;
		impl->sampleRate = sampleRate;
	}

	void audioConvertSampleRate(Audio *snd, uint32 sampleRate, uint32 quality)
	{
		const uint64 originalDuration = snd->duration();
		audioConvertFrames(snd, snd->frames() * sampleRate / snd->sampleRate(), quality);
		CAGE_ASSERT(abs((sint32)(snd->sampleRate() - sampleRate)) < 10);
		audioSetSampleRate(snd, sampleRate); // in case of rounding errors
		CAGE_ASSERT(abs((sint64)(snd->duration() - originalDuration)) < 100);
	}

	void audioConvertFrames(Audio *snd, uint64 frames, uint32 quality)
	{
		AudioImpl *impl = (AudioImpl *)snd;
		audioConvertFormat(snd, AudioFormatEnum::Float);
		MemoryBuffer tmp;
		tmp.resize(frames * snd->channels() * sizeof(float));
		SampleRateConverterCreateConfig cfg(snd->channels());
		cfg.quality = quality;
		Holder<SampleRateConverter> cnv = newSampleRateConverter(cfg);
		const uint32 targetSampleRate = numeric_cast<uint32>(1000000 * frames / snd->duration());
		cnv->convert(snd->rawViewFloat(), bufferCast<float, char>(tmp), targetSampleRate / (double)snd->sampleRate());
		std::swap(impl->mem, tmp);
		impl->sampleRate = targetSampleRate;
		impl->frames = frames;
	}

	void vorbisConvertFormat(AudioImpl *snd, AudioFormatEnum format);

	void audioConvertFormat(Audio *snd, AudioFormatEnum format)
	{
		CAGE_ASSERT(format != AudioFormatEnum::Default);
		AudioImpl *impl = (AudioImpl *)snd;
		if (impl->format == format)
			return; // no op
		
		if (impl->format == AudioFormatEnum::Vorbis || format == AudioFormatEnum::Vorbis)
			return vorbisConvertFormat(impl, format);

		Holder<Audio> tmp = newAudio();
		tmp->initialize(impl->frames, impl->channels, impl->sampleRate, format);
		audioBlit(impl, +tmp, 0, 0, impl->frames);
		AudioImpl *t = (AudioImpl *)+tmp;
		std::swap(impl->mem, t->mem);
		std::swap(impl->format, t->format);
	}

	namespace
	{
		bool overlaps(uint64 x1, uint64 y1, uint64 s)
		{
			if (x1 > y1)
				std::swap(x1, y1);
			uint64 x2 = x1 + s;
			uint64 y2 = y1 + s;
			return x1 < y2 && y1 < x2;
		}
	}

	Holder<Audio> vorbisExtract(const AudioImpl *snd, uint64 offset, uint64 frames);

	void audioBlit(const Audio *source, Audio *target, uint64 sourceFrameOffset, uint64 targetFrameOffset, uint64 frames)
	{
		const AudioImpl *s = (const AudioImpl *)source;
		AudioImpl *t = (AudioImpl *)target;

		if (t->format == AudioFormatEnum::Vorbis || (t->format == AudioFormatEnum::Default && s->format == AudioFormatEnum::Vorbis))
			CAGE_THROW_ERROR(Exception, "audioBlit into vorbis is forbidden");

		CAGE_ASSERT(s->format != AudioFormatEnum::Default && s->channels > 0);
		CAGE_ASSERT(s != t || !overlaps(sourceFrameOffset, targetFrameOffset, frames));
		if (t->format == AudioFormatEnum::Default && targetFrameOffset == 0)
			t->initialize(s->frames, s->channels, s->sampleRate, s->format);
		CAGE_ASSERT(s->channels == t->channels);
		CAGE_ASSERT(sourceFrameOffset + frames <= s->frames);
		CAGE_ASSERT(targetFrameOffset + frames <= t->frames);

		if (s->format == AudioFormatEnum::Vorbis)
		{
			Holder<Audio> tmp = vorbisExtract(s, sourceFrameOffset, frames);
			audioBlit(+tmp, target, 0, targetFrameOffset, frames);
		}
		else if (s->format == t->format)
		{
			const uint64 fb = s->channels * formatBytes(s->format);
			detail::memcpy((char *)t->mem.data() + targetFrameOffset * fb, (char *)s->mem.data() + sourceFrameOffset * fb, frames * fb);
		}
		else
		{
			for (uint32 f = 0; f < frames; f++)
				for (uint32 c = 0; c < s->channels; c++)
					t->value(targetFrameOffset + f, c, s->value(sourceFrameOffset + f, c));
		}
	}
}
