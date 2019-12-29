namespace cage
{
	class CAGE_API MixingBus : private Immovable
	{
	public:
		void addInput(MixingBus *bus);
		void removeInput(MixingBus *bus);
		void addOutput(MixingBus *bus);
		void removeOutput(MixingBus *bus);
		void clear();
	};

	CAGE_API Holder<MixingBus> newMixingBus(SoundContext *context);
}
