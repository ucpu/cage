#ifndef guard_speakerList_h_m1n5ewqa7r8fuj4
#define guard_speakerList_h_m1n5ewqa7r8fuj4

#include "core.h"

namespace cage
{
	struct CAGE_ENGINE_API SpeakerLayout
	{
		string name;
		uint32 channels;
		SpeakerLayout();
	};

	struct CAGE_ENGINE_API SpeakerSamplerate
	{
		uint32 minimum;
		uint32 maximum;
		SpeakerSamplerate();
	};

	class CAGE_ENGINE_API SpeakerDevice : private Immovable
	{
	public:
		string id() const;
		string name() const;
		bool raw() const;

		uint32 layoutsCount() const;
		uint32 currentLayout() const; // layout index
		const SpeakerLayout &layout(uint32 index) const;
		PointerRange<const SpeakerLayout> layouts() const;

		uint32 sampleratesCount() const;
		uint32 currentSamplerate() const; // actual sample rate
		const SpeakerSamplerate &samplerate(uint32 index) const;
		PointerRange<const SpeakerSamplerate> samplerates() const;
	};

	class CAGE_ENGINE_API SpeakerList : private Immovable
	{
	public:
		uint32 devicesCount() const;
		uint32 defaultDevice() const;
		const SpeakerDevice *device(uint32 index) const;
		Holder<PointerRange<const SpeakerDevice *>> devices() const;
	};

	CAGE_ENGINE_API Holder<SpeakerList> newSpeakerList(bool inputs = false);
}

#endif // guard_speakerList_h_m1n5ewqa7r8fuj4
