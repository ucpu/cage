namespace cage
{
	struct CAGE_API mixingFilterApi
	{
		soundDataBufferStruct output;
		Delegate<void(const soundDataBufferStruct&)> input;
	};

	class CAGE_API mixingFilter : private Immovable
	{
	public:
		void setBus(mixingBus *bus);
		Delegate<void(const mixingFilterApi&)> execute;
	};

	class CAGE_API volumeFilter : private Immovable
	{
	public:
		Holder<mixingFilter> filter;
		real volume;
	};

	CAGE_API Holder<mixingFilter> newMixingFilter(soundContext *context);
	CAGE_API Holder<volumeFilter> newVolumeFilter(soundContext *context);
}
