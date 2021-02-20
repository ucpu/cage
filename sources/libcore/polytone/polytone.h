#include <cage-core/memoryBuffer.h>
#include <cage-core/polytone.h>

namespace cage
{
	class PolytoneImpl : public Polytone
	{
	public:
		MemoryBuffer mem;
		uint64 frames = 0;
		uint32 channels = 0, sampleRate = 0;
		PolytoneFormatEnum format = PolytoneFormatEnum::Default;
	};

	uint32 formatBytes(PolytoneFormatEnum format);
}
