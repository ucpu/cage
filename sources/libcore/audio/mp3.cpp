#include <dr_libs/dr_mp3.h>

#include "audio.h"

namespace cage
{
	void mp3Decode(PointerRange<const char> inBuffer, AudioImpl *impl)
	{
		drmp3 mp3;
		if (!drmp3_init_memory(&mp3, inBuffer.data(), inBuffer.size(), nullptr))
			CAGE_THROW_ERROR(Exception, "failed to initialize decoding mp3 sound");
		try
		{
			const uint64 frames = drmp3_get_pcm_frame_count(&mp3);
#ifdef CAGE_ARCHITECTURE_32
			if (frames > (uint32)m)
				CAGE_THROW_ERROR(Exception, "32 bit build of cage cannot handle this large mp3 file")
#endif // CAGE_ARCHITECTURE_32
			impl->initialize(numeric_cast<uintPtr>(frames), mp3.channels, mp3.sampleRate, AudioFormatEnum::Float);
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

	MemoryBuffer mp3Encode(const AudioImpl *impl)
	{
		CAGE_THROW_CRITICAL(Exception, "encoding mp3 sound files");
	}
}
