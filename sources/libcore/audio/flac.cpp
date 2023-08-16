#include <dr_libs/dr_flac.h>

#include "audio.h"

namespace cage
{
	void flacDecode(PointerRange<const char> inBuffer, AudioImpl *impl)
	{
		drflac *flac = drflac_open_memory(inBuffer.data(), inBuffer.size(), nullptr);
		try
		{
#ifdef CAGE_ARCHITECTURE_32
			if (flac->totalPCMFrameCount > (uint32)m)
				CAGE_THROW_ERROR(Exception, "32 bit build of cage cannot handle this large flac file")
#endif // CAGE_ARCHITECTURE_32
			impl->initialize(numeric_cast<uintPtr>(flac->totalPCMFrameCount), flac->channels, flac->sampleRate, AudioFormatEnum::S32);
			if (drflac_read_pcm_frames_s32(flac, impl->frames, (sint32 *)impl->mem.data()) != impl->frames)
				CAGE_THROW_ERROR(Exception, "failed to read samples in decoding flac sound");
		}
		catch (...)
		{
			drflac_close(flac);
			throw;
		}
		drflac_close(flac);
	}

	MemoryBuffer flacEncode(const AudioImpl *impl)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "encoding flac sound files");
	}
}
