#ifdef CAGE_SYSTEM_MAC

	#include <csignal>

	#include <cage-core/process.h> // installSigTermHandler

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

	namespace
	{
		Delegate<void()> sigTermHandler;
		void sigTermHandlerEntry(int)
		{
			if (sigTermHandler)
				sigTermHandler();
		}

		Delegate<void()> sigIntHandler;
		void sigIntHandlerEntry(int)
		{
			if (sigIntHandler)
				sigIntHandler();
		}
	}

	void installSigTermHandler(Delegate<void()> handler)
	{
		sigTermHandler = handler;
		signal(SIGTERM, &sigTermHandlerEntry);
	}

	void installSigIntHandler(Delegate<void()> handler)
	{
		sigIntHandler = handler;
		signal(SIGINT, &sigIntHandlerEntry);
	}
}

#endif // CAGE_SYSTEM_MAC
