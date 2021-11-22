#include <cage-simple/engine.h>

namespace cage
{
	EntityComponent *transformHistoryComponent;

	EngineControlThread &controlThread()
	{
		static EngineControlThread instance;
		return instance;
	}

	EngineGraphicsDispatchThread &graphicsDispatchThread()
	{
		static EngineGraphicsDispatchThread instance;
		return instance;
	}

	EngineGraphicsPrepareThread &graphicsPrepareThread()
	{
		static EngineGraphicsPrepareThread instance;
		return instance;
	}

	EngineSoundThread &soundThread()
	{
		static EngineSoundThread instance;
		return instance;
	}
}
