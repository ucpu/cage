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
		PointerRange<const Real> axes() const;
		PointerRange<const bool> buttons() const;

		Real deadzone = 0.05;
	};

	CAGE_ENGINE_API Holder<Gamepad> newGamepad();

	CAGE_ENGINE_API uint32 gamepadsAvailable();
}

#endif
