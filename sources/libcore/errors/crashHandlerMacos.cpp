#ifdef CAGE_SYSTEM_MAC

	#include <cage-core/core.h>

namespace cage
{
	void crashHandlerThreadInit()
	{
		// nothing for now
	}

	namespace
	{
		struct SetupHandlers
		{
			SetupHandlers() { crashHandlerThreadInit(); }
		} setupHandlers;
	}
}

#endif // CAGE_SYSTEM_MAC
