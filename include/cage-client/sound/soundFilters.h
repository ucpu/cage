namespace cage
{
	template struct CAGE_API delegate<void(const soundDataBufferStruct&)>;

	struct CAGE_API filterApiStruct
	{
		soundDataBufferStruct output;
		delegate<void(const soundDataBufferStruct&)> input;
	};

	template struct CAGE_API delegate<void(const filterApiStruct&)>;

	class CAGE_API filterClass
	{
	public:
		void setBus(busClass *bus);
		delegate<void(const filterApiStruct&)> execute;
	};

	template struct CAGE_API holder<filterClass>;

	class CAGE_API volumeFilterClass
	{
	public:
		holder<filterClass> filter;
		real volume;
	};

	CAGE_API holder<filterClass> newFilter(soundContextClass *context);
	CAGE_API holder<volumeFilterClass> newFilterVolume(soundContextClass *context);
}
