namespace cage
{
	struct CAGE_API filterApiStruct
	{
		soundDataBufferStruct output;
		delegate<void(const soundDataBufferStruct&)> input;
	};

	class CAGE_API filterClass
	{
	public:
		void setBus(busClass *bus);
		delegate<void(const filterApiStruct&)> execute;
	};

	class CAGE_API volumeFilterClass
	{
	public:
		holder<filterClass> filter;
		real volume;
	};

	CAGE_API holder<filterClass> newFilter(soundContextClass *context);
	CAGE_API holder<volumeFilterClass> newFilterVolume(soundContextClass *context);
}
