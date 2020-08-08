#ifndef guard_speakerList_h_m1n5ewqa7r8fuj4
#define guard_speakerList_h_m1n5ewqa7r8fuj4

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API SpeakerDevice
	{
	public:
		string id;
		string name;
		string group;
		string vendor;

		uint32 channels;
		uint32 minSamplerate;
		uint32 maxSamplerate;
		uint32 defaultSamplerate;
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
