#include "polytone.h"

#include <cage-core/math.h>
#include <cage-core/serialization.h>

namespace cage
{
	uint32 formatBytes(PolytoneFormatEnum format)
	{
		switch (format)
		{
		case PolytoneFormatEnum::S16: return sizeof(sint16);
		case PolytoneFormatEnum::S32: return sizeof(sint32);
		case PolytoneFormatEnum::Float: return sizeof(float);
		default: CAGE_THROW_CRITICAL(Exception, "invalid polytone format");
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
			((sint16 *)impl->mem.data())[offset] = numeric_cast<sint16>(saturate(v) * 32767.f);
			break;
		case PolytoneFormatEnum::S32:
			((sint32 *)impl->mem.data())[offset] = numeric_cast<sint32>(saturate(v) * 2147483647.f);
			break;
		case PolytoneFormatEnum::Float:
			((float *)impl->mem.data())[offset] = v;
			break;
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

	void polytoneConvertFormat(Polytone *snd, PolytoneFormatEnum format)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "polytoneConvertFormat");
	}
}
