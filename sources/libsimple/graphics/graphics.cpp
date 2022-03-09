#include <cage-core/swapBufferGuard.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/entitiesCopy.h>

#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/opengl.h>
#include <cage-engine/window.h>

#include "graphics.h"

namespace cage
{
	Graphics::Graphics(const EngineCreateConfig &config)
	{
		{
			SwapBufferGuardCreateConfig cfg;
			cfg.buffersCount = 3;
			cfg.repeatedReads = true;
			emitBuffersGuard = newSwapBufferGuard(cfg);
		}
		emitBuffersMemory = newMemoryAllocatorStream({});
	}

	Graphics::~Graphics()
	{}

	void Graphics::initialize()
	{
		renderQueue = newRenderQueue();
		provisionalData = newProvisionalGraphics();
	}

	void Graphics::finalize()
	{
		renderQueue.clear();
		provisionalData.clear();
	}

	void Graphics::emit(uint64 time)
	{
		if (auto lock = emitBuffersGuard->write())
		{
			EntitiesCopyConfig cfg;
			cfg.source = engineEntities();
			cfg.destination = +emitBuffers[lock.index()].scene;
			entitiesCopy(cfg);
			emitBuffers[lock.index()].time = time;
		}
	}

	void Graphics::dispatch()
	{
		renderQueue->dispatch(+provisionalData);
		provisionalData->reset();

		{ // check gl errors (even in release, but do not halt the game)
			try
			{
				checkGlError();
			}
			catch (const GraphicsError &)
			{
				// nothing
			}
		}
	}

	void Graphics::swap()
	{
		CAGE_CHECK_GL_ERROR_DEBUG();
		engineWindow()->swapBuffers();
		glFinish(); // this is where the engine should be waiting for the gpu
	}

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
