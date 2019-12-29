namespace cage
{
	class CAGE_API Speaker : private Immovable
	{
	public:
		string getStreamName() const;
		string getDeviceId() const;
		string getDeviceName() const;
		bool getDeviceRaw() const;
		string getLayoutName() const;
		uint32 getChannelsCount() const;
		uint32 getOutputSampleRate() const;

		void setInput(MixingBus *bus);
		void update(uint64 time);

		float channelVolumes[16];
	};

	struct CAGE_API SpeakerCreateConfig
	{
		string deviceId;
		uint32 sampleRate;
		//uint32 channelsLayoutIndex;
		bool deviceRaw;
		SpeakerCreateConfig();
	};

	CAGE_API Holder<Speaker> newSpeakerOutput(SoundContext *context, const SpeakerCreateConfig &config, string name = "");
}
