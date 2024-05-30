#ifndef guard_audioChannelsConverter_h_56dtj4ew89a
#define guard_audioChannelsConverter_h_56dtj4ew89a

#include <cage-core/core.h>

namespace cage
{
	class CAGE_CORE_API AudioChannelsConverter : private Immovable
	{
	public:
		void convert(PointerRange<const float> src, PointerRange<float> dst, uint32 srcChannels, uint32 dstChannels);
	};

	CAGE_CORE_API Holder<AudioChannelsConverter> newAudioChannelsConverter();
}

#endif // guard_audioChannelsConverter_h_56dtj4ew89a
