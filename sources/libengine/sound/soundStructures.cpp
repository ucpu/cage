#include "private.h"

namespace cage
{
	SoundInterleavedBuffer::SoundInterleavedBuffer()
	{}

	SoundInterleavedBuffer::~SoundInterleavedBuffer()
	{
		if (allocated)
			detail::systemArena().deallocate(buffer);
	};

	SoundInterleavedBuffer::SoundInterleavedBuffer(const SoundInterleavedBuffer &other) : buffer(other.buffer), frames(other.frames), channels(other.channels), allocated(0)
	{}

	SoundInterleavedBuffer &SoundInterleavedBuffer::operator = (const SoundInterleavedBuffer &other)
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

	void SoundInterleavedBuffer::resize(uint32 channels, uint32 frames)
	{
		CAGE_ASSERT(channels > 0 && frames > 0);
		CAGE_ASSERT(!!allocated == !!buffer);
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

	void SoundInterleavedBuffer::clear()
	{
		if (!buffer)
			return;
		detail::memset(buffer, 0, channels * frames * sizeof(float));
	}
}
