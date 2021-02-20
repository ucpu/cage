#include "polytone.h"

#include <dr_libs/dr_mp3.h>

namespace cage
{
	void mp3Decode(PointerRange<const char> inBuffer, PolytoneImpl *impl)
	{
		drmp3 mp3;
		if (!drmp3_init_memory(&mp3, inBuffer.data(), inBuffer.size(), nullptr))
			CAGE_THROW_ERROR(Exception, "failed to initialize decoding mp3 sound");
		try
		{
			impl->initialize(drmp3_get_pcm_frame_count(&mp3), mp3.channels, mp3.sampleRate, PolytoneFormatEnum::Float);
			if (drmp3_read_pcm_frames_f32(&mp3, impl->frames, (float *)impl->mem.data()) != impl->frames)
				CAGE_THROW_ERROR(Exception, "failed to read samples in decoding mp3 sound");
		}
		catch (...)
		{
			drmp3_uninit(&mp3);
			throw;
		}
		drmp3_uninit(&mp3);
	}

	MemoryBuffer mp3Encode(const PolytoneImpl *impl)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "encoding mp3 sound files");
	}
}
