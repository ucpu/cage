namespace cage
{
	struct CAGE_API screenMode
	{
	public:
		screenMode();
		pointStruct resolution;
		uint32 frequency;
	};

	class CAGE_API screenDeviceClass
	{
	public:
		uint32 modesCount() const;
		uint32 currentMode() const;
		const screenMode &mode(uint32 index) const;
		pointerRange<const screenMode> modes() const;
		string name() const;
		string id() const;
	};

	class CAGE_API screenListClass
	{
	public:
		uint32 devicesCount() const;
		uint32 primaryDevice() const;
		const screenDeviceClass &device(uint32 index) const;
		pointerRange<const screenDeviceClass> devices() const;
	};

	CAGE_API holder<screenListClass> newScreenList();
}
