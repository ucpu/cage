namespace cage
{
	class CAGE_API soundSource : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void setDataNone();
		void setDataRaw(uint32 channels, uint32 frames, uint32 sampleRate, const float *data);
		void setDataVorbis(uintPtr size, const void *buffer);
		void setDataTone(uint32 pitch = 440);
		void setDataNoise();

		void setDataRepeat(bool repeatBeforeStart, bool repeatAfterEnd);

		void addOutput(mixingBus *bus);
		void removeOutput(mixingBus *bus);

		uint64 getDuration() const;
		uint32 getChannels() const;
		uint32 getSampleRate() const;
	};

	CAGE_API Holder<soundSource> newSoundSource(soundContext *context);

	CAGE_API AssetScheme genAssetSchemeSoundSource(uint32 threadIndex, soundContext *memoryContext);
	static const uint32 assetSchemeIndexSoundSource = 20;
}
