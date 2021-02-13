#include "polytone.h"

#include <dr_libs/dr_flac.h>

namespace cage
{
	void flacDecode(PointerRange<const char> inBuffer, PolytoneImpl *impl)
	{
		drflac *flac = drflac_open_memory(inBuffer.data(), inBuffer.size(), nullptr);
		try
		{
			impl->sampleRate = flac->sampleRate;
			impl->frames = flac->totalPCMFrameCount;
			impl->channels = flac->channels;
			impl->format = PolytoneFormatEnum::S32;
			impl->mem.resize(impl->frames * impl->channels * sizeof(sint32));
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

	MemoryBuffer flacEncode(const PolytoneImpl *impl)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "encoding flac sound files");
	}
}
