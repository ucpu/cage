namespace cage
{
	class CAGE_API speakerClass
	{
	public:
		string getStreamName() const;
		string getDeviceId() const;
		string getDeviceName() const;
		bool getDeviceRaw() const;
		string getLayoutName() const;
		uint32 getChannelsCount() const;
		uint32 getOutputSampleRate() const;

		void setInput(busClass *bus);
		void update(uint64 time);

		float channelVolumes[16];
	};

	struct CAGE_API speakerCreateConfig
	{
		string deviceId;
		uint32 sampleRate;
		//uint32 channelsLayoutIndex;
		bool deviceRaw;
		speakerCreateConfig();
	};

	CAGE_API holder<speakerClass> newSpeaker(soundContextClass *context, const speakerCreateConfig &config, string name = "");
}
