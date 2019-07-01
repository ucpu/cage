#ifndef guard_speakerList_h_m1n5ewqa7r8fuj4
#define guard_speakerList_h_m1n5ewqa7r8fuj4

namespace cage
{
	struct CAGE_API speakerLayout
	{
		string name;
		uint32 channels;
		speakerLayout();
	};

	struct CAGE_API speakerSamplerate
	{
		uint32 minimum;
		uint32 maximum;
		speakerSamplerate();
	};

	class CAGE_API speakerDeviceClass : private immovable
	{
	public:
		string id() const;
		string name() const;
		bool raw() const;

		uint32 layoutsCount() const;
		uint32 currentLayout() const; // layout index
		const speakerLayout &layout(uint32 index) const;
		pointerRange<const speakerLayout> layouts() const;

		uint32 sampleratesCount() const;
		uint32 currentSamplerate() const; // actual sample rate
		const speakerSamplerate &samplerate(uint32 index) const;
		pointerRange<const speakerSamplerate> samplerates() const;
	};

	class CAGE_API speakerListClass : private immovable
	{
	public:
		uint32 devicesCount() const;
		uint32 defaultDevice() const;
		const speakerDeviceClass *device(uint32 index) const;
		holder<pointerRange<const speakerDeviceClass *>> devices() const;
	};

	CAGE_API holder<speakerListClass> newSpeakerList(bool inputs = false);
}

#endif // guard_speakerList_h_m1n5ewqa7r8fuj4
