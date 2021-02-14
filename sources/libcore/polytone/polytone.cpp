#include "polytone.h"

#include <cage-core/math.h>
#include <cage-core/serialization.h>

#include <utility> // std::swap

namespace cage
{
	uint32 formatBytes(PolytoneFormatEnum format)
	{
		switch (format)
		{
		case PolytoneFormatEnum::S16: return sizeof(sint16);
		case PolytoneFormatEnum::S32: return sizeof(sint32);
		case PolytoneFormatEnum::Float: return sizeof(float);
		case PolytoneFormatEnum::Vorbis:
			CAGE_THROW_ERROR(Exception, "vorbis encoding has variable bitrate");
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid polytone format");
		}
	}

	void Polytone::clear()
	{
		PolytoneImpl *impl = (PolytoneImpl *)this;
		impl->frames = 0;
		impl->channels = 0;
		impl->sampleRate = 0;
		impl->format = PolytoneFormatEnum::Default;
		impl->mem.resize(0);
	}

	void Polytone::initialize(uint64 frames, uint32 channels, uint32 sampleRate, PolytoneFormatEnum format)
	{
		CAGE_ASSERT(format != PolytoneFormatEnum::Vorbis);
		CAGE_ASSERT(format != PolytoneFormatEnum::Default);
		CAGE_ASSERT(channels > 0);
		PolytoneImpl *impl = (PolytoneImpl *)this;
		impl->frames = frames;
		impl->channels = channels;
		impl->sampleRate = sampleRate;
		impl->format = format;
		impl->mem.resize(0); // avoid unnecessary copies without deallocating the memory
		impl->mem.resize(frames * channels * formatBytes(format));
		impl->mem.zero();
	}

	Holder<Polytone> Polytone::copy() const
	{
		const PolytoneImpl *src = (const PolytoneImpl *)this;
		Holder<Polytone> poly = newPolytone();
		PolytoneImpl *dst = (PolytoneImpl *)+poly;
		dst->mem = src->mem.copy();
		dst->frames = src->frames;
		dst->channels = src->channels;
		dst->sampleRate = src->sampleRate;
		dst->format = src->format;
		return poly;
	}

	void Polytone::importRaw(PointerRange<const char> buffer, uint64 frames, uint32 channels, uint32 sampleRate, PolytoneFormatEnum format)
	{
		PolytoneImpl *impl = (PolytoneImpl *)this;
		CAGE_ASSERT(buffer.size() >= frames * channels * formatBytes(format));
		initialize(frames, channels, sampleRate, format);
		detail::memcpy(impl->mem.data(), buffer.data(), impl->mem.size());
	}

	PointerRange<const sint16> Polytone::rawViewS16() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		CAGE_ASSERT(impl->format == PolytoneFormatEnum::S16);
		return bufferCast<const sint16, const char>(impl->mem);
	}

	PointerRange<const sint32> Polytone::rawViewS32() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		CAGE_ASSERT(impl->format == PolytoneFormatEnum::S32);
		return bufferCast<const sint32, const char>(impl->mem);
	}

	PointerRange<const float> Polytone::rawViewFloat() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		CAGE_ASSERT(impl->format == PolytoneFormatEnum::Float);
		return bufferCast<const float, const char>(impl->mem);
	}

	PointerRange<const char> Polytone::rawViewVorbis() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		CAGE_ASSERT(impl->format == PolytoneFormatEnum::Vorbis);
		return impl->mem;
	}

	uint64 Polytone::frames() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		return impl->frames;
	}

	uint32 Polytone::channels() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		return impl->channels;
	}

	uint32 Polytone::sampleRate() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		return impl->sampleRate;
	}

	PolytoneFormatEnum Polytone::format() const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		return impl->format;
	}

	float Polytone::value(uint64 f, uint32 c) const
	{
		const PolytoneImpl *impl = (const PolytoneImpl *)this;
		CAGE_ASSERT(f < impl->frames && c < impl->channels);
		const uint64 offset = f * impl->channels + c;
		switch (impl->format)
		{
		case PolytoneFormatEnum::S16:
			return ((sint16 *)impl->mem.data())[offset] / 32767.f;
		case PolytoneFormatEnum::S32:
			return ((sint32 *)impl->mem.data())[offset] / 2147483647.f;
		case PolytoneFormatEnum::Float:
			return ((float *)impl->mem.data())[offset];
		case PolytoneFormatEnum::Vorbis:
			CAGE_THROW_ERROR(Exception, "cannot directly sample vorbis encoded polytone");
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid polytone format");
		}
	}

	void Polytone::value(uint64 f, uint32 c, float v)
	{
		PolytoneImpl *impl = (PolytoneImpl *)this;
		CAGE_ASSERT(f < impl->frames &&c < impl->channels);
		const uint64 offset = f * impl->channels + c;
		switch (impl->format)
		{
		case PolytoneFormatEnum::S16:
			((sint16 *)impl->mem.data())[offset] = numeric_cast<sint16>(clamp(real(v), -1, 1) * 32767.f);
			break;
		case PolytoneFormatEnum::S32:
			((sint32 *)impl->mem.data())[offset] = numeric_cast<sint32>(clamp(real(v), -1, 1) * 2147483647.f);
			break;
		case PolytoneFormatEnum::Float:
			((float *)impl->mem.data())[offset] = v;
			break;
		case PolytoneFormatEnum::Vorbis:
			CAGE_THROW_ERROR(Exception, "cannot directly sample vorbis encoded polytone");
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid polytone format");
		}
	}

	void Polytone::value(uint64 f, uint32 c, const real &v) { value(f, c, v.value); }

	Holder<Polytone> newPolytone()
	{
		return detail::systemArena().createImpl<Polytone, PolytoneImpl>();
	}

	void polytoneSetSampleRate(Polytone *snd, uint32 sampleRate)
	{
		PolytoneImpl *impl = (PolytoneImpl *)snd;
		impl->sampleRate = sampleRate;
	}

	void polytoneConvertChannels(Polytone *snd, uint32 channels)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "polytoneConvertChannels");
	}

	void vorbisConvertFormat(PolytoneImpl *snd, PolytoneFormatEnum format);

	void polytoneConvertFormat(Polytone *snd, PolytoneFormatEnum format)
	{
		CAGE_ASSERT(format != PolytoneFormatEnum::Default);
		PolytoneImpl *impl = (PolytoneImpl *)snd;
		if (impl->format == format)
			return; // no op
		
		if (impl->format == PolytoneFormatEnum::Vorbis || format == PolytoneFormatEnum::Vorbis)
			return vorbisConvertFormat(impl, format);

		Holder<Polytone> tmp = newPolytone();
		tmp->initialize(impl->frames, impl->channels, impl->sampleRate, format);
		polytoneBlit(impl, +tmp, 0, 0, impl->frames);
		PolytoneImpl *t = (PolytoneImpl *)+tmp;
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

	Holder<Polytone> vorbisExtract(const PolytoneImpl *snd, uint64 offset, uint64 frames);

	void polytoneBlit(const Polytone *source, Polytone *target, uint64 sourceFrameOffset, uint64 targetFrameOffset, uint64 frames)
	{
		const PolytoneImpl *s = (const PolytoneImpl *)source;
		PolytoneImpl *t = (PolytoneImpl *)target;

		if (t->format == PolytoneFormatEnum::Vorbis || (t->format == PolytoneFormatEnum::Default && s->format == PolytoneFormatEnum::Vorbis))
			CAGE_THROW_ERROR(Exception, "polytoneBlit into vorbis is forbidden");

		CAGE_ASSERT(s->format != PolytoneFormatEnum::Default && s->channels > 0);
		CAGE_ASSERT(s != t || !overlaps(sourceFrameOffset, targetFrameOffset, frames));
		if (t->format == PolytoneFormatEnum::Default && targetFrameOffset == 0)
			t->initialize(s->frames, s->channels, s->sampleRate, s->format);
		CAGE_ASSERT(s->channels == t->channels);
		CAGE_ASSERT(sourceFrameOffset + frames <= s->frames);
		CAGE_ASSERT(targetFrameOffset + frames <= t->frames);

		if (s->format == PolytoneFormatEnum::Vorbis)
		{
			Holder<Polytone> tmp = vorbisExtract(s, sourceFrameOffset, frames);
			polytoneBlit(+tmp, target, 0, targetFrameOffset, frames);
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
