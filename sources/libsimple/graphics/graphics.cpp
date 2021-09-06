#include <cage-core/swapBufferGuard.h>
#include <cage-core/memoryAllocators.h>

#include "graphics.h"

namespace cage
{
	Graphics::Graphics(const EngineCreateConfig &config)
	{
		{
			SwapBufferGuardCreateConfig cfg(3);
			cfg.repeatedReads = true;
			emitBuffersGuard = newSwapBufferGuard(cfg);
		}
		emitBuffersMemory = newMemoryAllocatorStream({});
	}

	Graphics::~Graphics()
	{}

	void graphicsCreate(const EngineCreateConfig &config)
	{
		CAGE_ASSERT(!graphics);
		graphics = systemMemory().createObject<Graphics>(config);
	}

	void graphicsDestroy()
	{
		systemMemory().destroy<Graphics>(graphics);
		graphics = nullptr;
	}

	void graphicsInitialize()
	{
		graphics->initialize();
	}

	void graphicsFinalize()
	{
		graphics->finalize();
	}

	void graphicsEmit(uint64 time)
	{
		graphics->emit(time);
	}

	void graphicsPrepare(uint64 time, uint32 &drawCalls, uint32 &drawPrimitives)
	{
		graphics->prepare(time);
		drawCalls = graphics->outputDrawCalls;
		drawPrimitives = graphics->outputDrawPrimitives;
	}

	void graphicsDispatch()
	{
		graphics->dispatch();
	}

	void graphicsSwap()
	{
		graphics->swap();
	}

	Graphics *graphics;
}
