namespace cage
{
	class CAGE_API speakerListClass : private immovable
	{
	public:
		uint32 deviceCount() const;
		uint32 deviceDefault() const; // returns device index
		string deviceId(uint32 device) const;
		string deviceName(uint32 device) const;
		bool deviceRaw(uint32 device) const;

		uint32 layoutCount(uint32 device) const;
		uint32 layoutCurrent(uint32 device) const; // returns layout index
		string layoutName(uint32 device, uint32 layout) const;
		uint32 layoutChannels(uint32 device, uint32 layout) const;

		uint32 samplerateCount(uint32 device) const;
		uint32 samplerateCurrent(uint32 device) const; // returns actual sample rate
		uint32 samplerateMin(uint32 device, uint32 samplerate) const;
		uint32 samplerateMax(uint32 device, uint32 samplerate) const;
	};

	CAGE_API holder<speakerListClass> newSpeakerList(bool inputs = false);
}
