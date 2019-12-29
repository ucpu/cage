namespace cage
{
	struct CAGE_API MixingFilterApi
	{
		SoundDataBuffer output;
		Delegate<void(const SoundDataBuffer&)> input;
	};

	class CAGE_API MixingFilter : private Immovable
	{
	public:
		void setBus(MixingBus *bus);
		Delegate<void(const MixingFilterApi&)> execute;
	};

	class CAGE_API VolumeFilter : private Immovable
	{
	public:
		Holder<MixingFilter> filter;
		real volume;
	};

	CAGE_API Holder<MixingFilter> newMixingFilter(SoundContext *context);
	CAGE_API Holder<VolumeFilter> newVolumeFilter(SoundContext *context);
}
