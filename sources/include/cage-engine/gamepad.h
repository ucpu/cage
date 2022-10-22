#ifndef guard_gamepad_oi56awe41g7
#define guard_gamepad_oi56awe41g7

#include <cage-engine/inputs.h>

namespace cage
{
	class CAGE_ENGINE_API Gamepad : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();

		String name() const;
		bool connected() const;
		bool mapped() const;
		PointerRange<const Real> axes() const; // left stick x, y, right stick x, y, left trigger, right trigger
		PointerRange<const bool> buttons() const; // a, b, x, y, left shoulder, right shoulder, select, menu, _, _, _, up, right, down, left

		Real deadzone = 0.05;
	};

	CAGE_ENGINE_API Holder<Gamepad> newGamepad();

	CAGE_ENGINE_API uint32 gamepadsAvailable();
}

#endif
