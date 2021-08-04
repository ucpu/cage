#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/opengl.h>
#include <cage-engine/window.h>

#include "graphics.h"

namespace cage
{
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
}
