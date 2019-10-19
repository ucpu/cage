namespace cage
{
	struct CAGE_API mixingFilterApi
	{
		soundDataBufferStruct output;
		delegate<void(const soundDataBufferStruct&)> input;
	};

	class CAGE_API mixingFilter : private immovable
	{
	public:
		void setBus(mixingBus *bus);
		delegate<void(const mixingFilterApi&)> execute;
	};

	class CAGE_API volumeFilter : private immovable
	{
	public:
		holder<mixingFilter> filter;
		real volume;
	};

	CAGE_API holder<mixingFilter> newMixingFilter(soundContext *context);
	CAGE_API holder<volumeFilter> newVolumeFilter(soundContext *context);
}
