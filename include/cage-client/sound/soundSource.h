namespace cage
{
	class CAGE_API sourceClass
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

		void addOutput(busClass *bus);
		void removeOutput(busClass *bus);

		uint64 getDuration() const;
		uint32 getChannels() const;
		uint32 getSampleRate() const;
	};

	CAGE_API holder<sourceClass> newSource(soundContextClass *context);
}
