#include "audio.h"

#include <cage-core/audioAlgorithms.h>
#include <cage-core/math.h>
#include <cage-core/sampleRateConverter.h>
#include <cage-core/serialization.h>

namespace cage
{
	uintPtr formatBytes(AudioFormatEnum format)
	{
		switch (format)
		{
			case AudioFormatEnum::S16:
				return sizeof(sint16);
			case AudioFormatEnum::S32:
				return sizeof(sint32);
			case AudioFormatEnum::Float:
				return sizeof(float);
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

	void Audio::initialize(uintPtr frames, uint32 channels, uint32 sampleRate, AudioFormatEnum format)
	{
		if (format == AudioFormatEnum::Default || format == AudioFormatEnum::Vorbis)
			CAGE_THROW_ERROR(Exception, "cannot initialize audio with default or vorbis format");
		if (channels == 0)
			CAGE_THROW_ERROR(Exception, "cannot initialize audio with no channels");
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

	void Audio::importRaw(PointerRange<const char> buffer, uintPtr frames, uint32 channels, uint32 sampleRate, AudioFormatEnum format)
	{
		if (buffer.size() < (uintPtr)frames * channels * formatBytes(format))
			CAGE_THROW_ERROR(Exception, "audio raw import with insufficient buffer");
		AudioImpl *impl = (AudioImpl *)this;
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

	uintPtr Audio::frames() const
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
		return uint64(1000000) * frames() / sampleRate();
	}

	float Audio::value(uintPtr f, uint32 c) const
	{
		const AudioImpl *impl = (const AudioImpl *)this;
		CAGE_ASSERT(f < impl->frames && c < impl->channels);
		const uintPtr offset = f * impl->channels + c;
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

	void Audio::value(uintPtr f, uint32 c, float v)
	{
		AudioImpl *impl = (AudioImpl *)this;
		CAGE_ASSERT(f < impl->frames && c < impl->channels);
		const uintPtr offset = f * impl->channels + c;
		switch (impl->format)
		{
			case AudioFormatEnum::S16:
				((sint16 *)impl->mem.data())[offset] = numeric_cast<sint16>(clamp(Real(v), -1, 1) * 32767.f);
				break;
			case AudioFormatEnum::S32:
				((sint32 *)impl->mem.data())[offset] = numeric_cast<sint32>(clamp(Real(v), -1, 1) * 2147483647.f);
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

	void Audio::value(uintPtr f, uint32 c, const Real &v)
	{
		value(f, c, v.value);
	}

	Holder<Audio> newAudio()
	{
		return systemMemory().createImpl<Audio, AudioImpl>();
	}

	void audioSetSampleRate(Audio *snd, uint32 sampleRate)
	{
		AudioImpl *impl = (AudioImpl *)snd;
		impl->sampleRate = sampleRate;
	}

	void audioConvertSampleRate(Audio *snd, uint32 sampleRate, uint32 quality)
	{
		const uint64 originalDuration = snd->duration();
		audioConvertFrames(snd, numeric_cast<uintPtr>(snd->frames() * uint64(sampleRate) / snd->sampleRate()), quality);
		audioSetSampleRate(snd, sampleRate); // in case of rounding errors
	}

	void audioConvertFrames(Audio *snd, uintPtr frames, uint32 quality)
	{
		AudioImpl *impl = (AudioImpl *)snd;
		audioConvertFormat(snd, AudioFormatEnum::Float);
		MemoryBuffer tmp;
		tmp.resize(frames * snd->channels() * sizeof(float));
		SampleRateConverterCreateConfig cfg(snd->channels());
		cfg.quality = quality;
		Holder<SampleRateConverter> cnv = newSampleRateConverter(cfg);
		const uint32 targetSampleRate = numeric_cast<uint32>(uint64(1000000) * frames / snd->duration());
		cnv->convert(snd->rawViewFloat(), bufferCast<float, char>(tmp), targetSampleRate / (double)snd->sampleRate());
		std::swap(impl->mem, tmp);
		impl->sampleRate = targetSampleRate;
		impl->frames = frames;
	}

	void vorbisConvertFormat(AudioImpl *snd, AudioFormatEnum format);

	void audioConvertFormat(Audio *snd, AudioFormatEnum format)
	{
		if (format == AudioFormatEnum::Default)
			CAGE_THROW_ERROR(Exception, "cannot convert audio to default format");
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

	void audioGain(Audio *snd, Real gain)
	{
		if (!valid(gain) || gain < 1e-5)
			CAGE_THROW_ERROR(Exception, "invalid gain");
		audioConvertFormat(snd, AudioFormatEnum::Float);
		for (float &f : snd->rawViewFloat().cast<float>())
			f *= gain.value;
	}

	namespace
	{
		bool overlaps(uintPtr x1, uintPtr y1, uintPtr s)
		{
			if (x1 > y1)
				std::swap(x1, y1);
			uintPtr x2 = x1 + s;
			uintPtr y2 = y1 + s;
			return x1 < y2 && y1 < x2;
		}
	}

	Holder<Audio> vorbisExtract(const AudioImpl *snd, uintPtr offset, uintPtr frames);

	void audioBlit(const Audio *source, Audio *target, uintPtr sourceFrameOffset, uintPtr targetFrameOffset, uintPtr frames)
	{
		if (frames == 0)
			return;

		const AudioImpl *s = (const AudioImpl *)source;
		AudioImpl *t = (AudioImpl *)target;

		if (t->format == AudioFormatEnum::Vorbis || (t->format == AudioFormatEnum::Default && s->format == AudioFormatEnum::Vorbis))
			CAGE_THROW_ERROR(Exception, "audioBlit into vorbis is not possible");

		if (s->format == AudioFormatEnum::Default || s->channels == 0)
			CAGE_THROW_ERROR(Exception, "audioBlit from default format or no channels");
		if (s == t && overlaps(sourceFrameOffset, targetFrameOffset, frames))
			CAGE_THROW_ERROR(Exception, "audioBlit with overlapping source and destination");
		if (t->format == AudioFormatEnum::Default && targetFrameOffset == 0)
			t->initialize(s->frames, s->channels, s->sampleRate, s->format);
		if (s->channels != t->channels)
			CAGE_THROW_ERROR(Exception, "audioBlit with different number of channels");
		if (sourceFrameOffset + frames > s->frames || targetFrameOffset + frames > t->frames)
			CAGE_THROW_ERROR(Exception, "audioBlit outside buffer size");

		if (s->format == AudioFormatEnum::Vorbis)
		{
			Holder<Audio> tmp = vorbisExtract(s, sourceFrameOffset, frames);
			audioBlit(+tmp, target, 0, targetFrameOffset, frames);
		}
		else if (s->format == t->format)
		{
			const uintPtr fb = s->channels * formatBytes(s->format);
			detail::memcpy((char *)t->mem.data() + targetFrameOffset * fb, (char *)s->mem.data() + sourceFrameOffset * fb, frames * fb);
		}
		else
		{
			for (uintPtr f = 0; f < frames; f++)
				for (uint32 c = 0; c < s->channels; c++)
					t->value(targetFrameOffset + f, c, s->value(sourceFrameOffset + f, c));
		}
	}
}
