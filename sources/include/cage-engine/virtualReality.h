#ifndef guard_virtualReality_h_dt4fuj1iu4ftc
#define guard_virtualReality_h_dt4fuj1iu4ftc

#include <cage-engine/inputs.h>

namespace cage
{
	class CAGE_ENGINE_API VirtualReality : private Immovable
	{
	public:
		EventDispatcher<bool(const GenericInput &)> events;

		void processEvents();
	};

	CAGE_ENGINE_API Holder<VirtualReality> newVirtualReality();
}

#endif
