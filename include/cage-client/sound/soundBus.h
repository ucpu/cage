namespace cage
{
	class CAGE_API busClass
	{
	public:
		void addInput(busClass *bus);
		void removeInput(busClass *bus);
		void addOutput(busClass *bus);
		void removeOutput(busClass *bus);
		void clear();
	};

	CAGE_API holder<busClass> newBus(soundContextClass *context);
}
