namespace cage
{
	class CAGE_API speakerOutput : private immovable
	{
	public:
		string getStreamName() const;
		string getDeviceId() const;
		string getDeviceName() const;
		bool getDeviceRaw() const;
		string getLayoutName() const;
		uint32 getChannelsCount() const;
		uint32 getOutputSampleRate() const;

		void setInput(mixingBus *bus);
		void update(uint64 time);

		float channelVolumes[16];
	};

	struct CAGE_API speakerOutputCreateConfig
	{
		string deviceId;
		uint32 sampleRate;
		//uint32 channelsLayoutIndex;
		bool deviceRaw;
		speakerOutputCreateConfig();
	};

	CAGE_API holder<speakerOutput> newSpeakerOutput(soundContext *context, const speakerOutputCreateConfig &config, string name = "");
}
