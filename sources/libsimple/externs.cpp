#include <cage-engine/inputs.h>
#include <cage-simple/engine.h>

namespace cage
{
	class EntityComponent;

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

	EventDispatcher<bool(const GenericInput &)> &engineEvents()
	{
		static EventDispatcher<bool(const GenericInput &)> instance;
		return instance;
	}

	EngineDynamicResolution &engineDynamicResolution()
	{
		static EngineDynamicResolution instance;
		return instance;
	}
}
