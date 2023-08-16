#ifndef guard_soundCommon_h_dokzt5456rtz45
#define guard_soundCommon_h_dokzt5456rtz45

#include <cage-engine/core.h>

namespace cage
{
	struct CAGE_ENGINE_API SoundCallbackData
	{
		PointerRange<float> buffer;
		sint64 time = 0;
		uint32 channels = 0;
		uint32 frames = 0;
		uint32 sampleRate = 0;
	};
}

#endif // guard_soundCommon_h_dokzt5456rtz45
