#ifndef guard_screenList_h_q56ew4rk87h64847
#define guard_screenList_h_q56ew4rk87h64847

namespace cage
{
	struct CAGE_API screenMode
	{
	public:
		screenMode();
		pointStruct resolution;
		uint32 frequency;
	};

	class CAGE_API screenDeviceClass // : private immovable
	{
	public:
		uint32 modesCount() const;
		uint32 currentMode() const;
		const screenMode &mode(uint32 index) const;
		pointerRange<const screenMode> modes() const;
		string name() const;
		string id() const;
	};

	class CAGE_API screenListClass : private immovable
	{
	public:
		uint32 devicesCount() const;
		uint32 primaryDevice() const;
		const screenDeviceClass &device(uint32 index) const;
		//holder<pointerRange<const screenDeviceClass*>> devices() const;
	};

	CAGE_API holder<screenListClass> newScreenList();
}

#endif // guard_screenList_h_q56ew4rk87h64847
