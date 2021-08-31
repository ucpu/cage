#ifndef guard_speakerList_h_m1n5ewqa7r8fuj4
#define guard_speakerList_h_m1n5ewqa7r8fuj4

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API SpeakerDevice
	{
	public:
		String id;
		String name;
		String group;
		String vendor;

		uint32 channels = 0;
		uint32 minSamplerate = 0;
		uint32 maxSamplerate = 0;
		uint32 defaultSamplerate = 0;

		bool available = false;
	};

	class CAGE_ENGINE_API SpeakerList : private Immovable
	{
	public:
		uint32 defaultDevice() const;
		PointerRange<const SpeakerDevice> devices() const;
	};

	CAGE_ENGINE_API Holder<SpeakerList> newSpeakerList();
}

#endif // guard_speakerList_h_m1n5ewqa7r8fuj4
