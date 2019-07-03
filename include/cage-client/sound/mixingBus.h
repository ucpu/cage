namespace cage
{
	class CAGE_API mixingBus : private immovable
	{
	public:
		void addInput(mixingBus *bus);
		void removeInput(mixingBus *bus);
		void addOutput(mixingBus *bus);
		void removeOutput(mixingBus *bus);
		void clear();
	};

	CAGE_API holder<mixingBus> newMixingBus(soundContext *context);
}
