namespace cage
{
	class CAGE_API soundSource : private immovable
	{
#ifdef CAGE_DEBUG
		detail::stringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void setDataNone();
		void setDataRaw(uint32 channels, uint32 frames, uint32 sampleRate, float *data);
		void setDataVorbis(uintPtr size, void *buffer);
		void setDataTone(uint32 pitch = 440);
		void setDataNoise();

		void setDataRepeat(bool repeatBeforeStart, bool repeatAfterEnd);

		void addOutput(mixingBus *bus);
		void removeOutput(mixingBus *bus);

		uint64 getDuration() const;
		uint32 getChannels() const;
		uint32 getSampleRate() const;
	};

	CAGE_API holder<soundSource> newSoundSource(soundContext *context);

	CAGE_API assetScheme genAssetSchemeSoundSource(uint32 threadIndex, soundContext *memoryContext);
	static const uint32 assetSchemeIndexSoundSource = 20;
}
