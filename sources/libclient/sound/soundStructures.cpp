#include <cage-core/core.h>
#include <cage-core/math.h>
#include "private.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>

namespace cage
{
	soundInterleavedBufferStruct::soundInterleavedBufferStruct() : buffer(nullptr), frames(0), channels(0), allocated(0)
	{}

	soundInterleavedBufferStruct::~soundInterleavedBufferStruct()
	{
		if (allocated)
			detail::systemArena().deallocate(buffer);
	};

	soundInterleavedBufferStruct::soundInterleavedBufferStruct(const soundInterleavedBufferStruct &other) : buffer(other.buffer), frames(other.frames), channels(other.channels), allocated(0)
	{}

	soundInterleavedBufferStruct &soundInterleavedBufferStruct::operator = (const soundInterleavedBufferStruct &other)
	{
		if (this == &other)
			return *this;
		if (allocated)
			detail::systemArena().deallocate(buffer);
		buffer = other.buffer;
		frames = other.frames;
		channels = other.channels;
		allocated = 0;
		return *this;
	}

	void soundInterleavedBufferStruct::resize(uint32 channels, uint32 frames)
	{
		CAGE_ASSERT_RUNTIME(channels > 0 && frames > 0, channels, frames);
		CAGE_ASSERT_RUNTIME(!!allocated == !!buffer, allocated, buffer);
		this->channels = channels;
		this->frames = frames;
		uint32 requested = channels * frames;
		if (allocated >= requested)
			return;
		if (buffer)
			detail::systemArena().deallocate(buffer);
		allocated = requested;
		buffer = (float*)detail::systemArena().allocate(allocated * sizeof(float), sizeof(uintPtr));
		if (!buffer)
			checkSoundIoError(SoundIoErrorNoMem);
	}

	void soundInterleavedBufferStruct::clear()
	{
		if (!buffer)
			return;
		detail::memset(buffer, 0, channels * frames * sizeof(float));
	}

	soundDataBufferStruct::soundDataBufferStruct() : time(0), sampleRate(0)
	{}
}
