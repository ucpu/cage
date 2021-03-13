#include <cage-core/memoryBuffer.h>
#include <cage-core/audio.h>

namespace cage
{
	class AudioImpl : public Audio
	{
	public:
		MemoryBuffer mem;
		uint64 frames = 0;
		uint32 channels = 0, sampleRate = 0;
		AudioFormatEnum format = AudioFormatEnum::Default;
	};

	uint32 formatBytes(AudioFormatEnum format);
}
