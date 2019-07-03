#ifndef guard_screenList_h_q56ew4rk87h64847
#define guard_screenList_h_q56ew4rk87h64847

namespace cage
{
	struct CAGE_API screenMode
	{
	public:
		ivec2 resolution;
		uint32 frequency;
		screenMode();
	};

	class CAGE_API screenDevice : private immovable
	{
	public:
		string id() const;
		string name() const;

		uint32 modesCount() const;
		uint32 currentMode() const;
		const screenMode &mode(uint32 index) const;
		pointerRange<const screenMode> modes() const;
	};

	class CAGE_API screenList : private immovable
	{
	public:
		uint32 devicesCount() const;
		uint32 defaultDevice() const;
		const screenDevice *device(uint32 index) const;
		holder<pointerRange<const screenDevice *>> devices() const;
	};

	CAGE_API holder<screenList> newScreenList();
}

#endif // guard_screenList_h_q56ew4rk87h64847
