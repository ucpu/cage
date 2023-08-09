#include <dr_libs/dr_wav.h>

#include "audio.h"

#include <cage-core/audioAlgorithms.h>

namespace cage
{
	void wavDecode(PointerRange<const char> inBuffer, AudioImpl *impl)
	{
		drwav wav = {};
		if (!drwav_init_memory(&wav, inBuffer.data(), inBuffer.size(), nullptr))
			CAGE_THROW_ERROR(Exception, "failed to initialize decoding wav sound");
		try
		{
			CAGE_ASSERT(impl->format == AudioFormatEnum::Default);
#ifdef CAGE_ARCHITECTURE_32
			if (wav.totalPCMFrameCount > (uint32)m)
				CAGE_THROW_ERROR(Exception, "32 bit build of cage cannot handle this large mp3 file")
#endif // CAGE_ARCHITECTURE_32
			if (wav.translatedFormatTag == DR_WAVE_FORMAT_PCM)
			{
				switch (wav.bitsPerSample)
				{
					case 16:
						impl->initialize(numeric_cast<uintPtr>(wav.totalPCMFrameCount), wav.channels, wav.sampleRate, AudioFormatEnum::S16);
						if (drwav_read_pcm_frames_s16(&wav, impl->frames, (sint16 *)impl->mem.data()) != impl->frames)
							CAGE_THROW_ERROR(Exception, "failed to read s16 samples in decoding wav sound");
						break;
					case 32:
						impl->initialize(numeric_cast<uintPtr>(wav.totalPCMFrameCount), wav.channels, wav.sampleRate, AudioFormatEnum::S32);
						if (drwav_read_pcm_frames_s32(&wav, impl->frames, (sint32 *)impl->mem.data()) != impl->frames)
							CAGE_THROW_ERROR(Exception, "failed to read s32 samples in decoding wav sound");
						break;
				}
			}
			if (impl->format == AudioFormatEnum::Default)
			{
				impl->initialize(numeric_cast<uintPtr>(wav.totalPCMFrameCount), wav.channels, wav.sampleRate, AudioFormatEnum::Float);
				if (drwav_read_pcm_frames_f32(&wav, impl->frames, (float *)impl->mem.data()) != impl->frames)
					CAGE_THROW_ERROR(Exception, "failed to read f32 samples in decoding wav sound");
			}
		}
		catch (...)
		{
			drwav_uninit(&wav);
			throw;
		}
		drwav_uninit(&wav);
	}

	MemoryBuffer wavEncode(const AudioImpl *impl)
	{
		if (impl->format == AudioFormatEnum::Vorbis)
		{
			Holder<Audio> tmp = impl->copy();
			audioConvertFormat(+tmp, AudioFormatEnum::Float);
			return wavEncode((AudioImpl *)+tmp);
		}

		void *buffer = nullptr;
		std::size_t size = 0;
		drwav wav = {};
		drwav_data_format format = {};
		format.channels = impl->channels;
		switch (impl->format)
		{
			case AudioFormatEnum::S16:
				format.format = DR_WAVE_FORMAT_PCM;
				format.bitsPerSample = 16;
				break;
			case AudioFormatEnum::S32:
				format.format = DR_WAVE_FORMAT_PCM;
				format.bitsPerSample = 32;
				break;
			case AudioFormatEnum::Float:
				format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
				format.bitsPerSample = 32;
				break;
			default:
				CAGE_THROW_CRITICAL(NotImplemented, "wav encode unsupported format");
		}
		format.sampleRate = impl->sampleRate;
		if (!drwav_init_memory_write_sequential_pcm_frames(&wav, &buffer, &size, &format, impl->frames, nullptr))
			CAGE_THROW_ERROR(Exception, "failed to initialize encoding wav sound");
		try
		{
			if (drwav_write_pcm_frames(&wav, impl->frames, impl->mem.data()) != impl->frames)
				CAGE_THROW_ERROR(Exception, "failed to write samples in encoding wav sound");
			drwav_uninit(&wav);
			MemoryBuffer res;
			res.resize(size);
			detail::memcpy(res.data(), buffer, size);
			drwav_free(buffer, nullptr);
			return res;
		}
		catch (...)
		{
			drwav_uninit(&wav);
			drwav_free(buffer, nullptr);
			throw;
		}
	}
}
