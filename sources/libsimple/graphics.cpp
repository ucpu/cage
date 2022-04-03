#include <cage-core/swapBufferGuard.h>
#include <cage-core/entitiesCopy.h>
#include <cage-core/entities.h>

#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderPipeline.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h>
#include <cage-engine/scene.h>

#include "engine.h"
#include "interpolationTimingCorrector.h"

namespace cage
{
	namespace
	{
		struct EmitBuffer : private Immovable
		{
			Holder<RenderPipeline> pipeline;
			Holder<EntityManager> scene = newEntityManager();
			uint64 emitTime = 0;
		};

		class Graphics : private Immovable
		{
		public:
			explicit Graphics(const EngineCreateConfig &config)
			{
				SwapBufferGuardCreateConfig cfg;
				cfg.buffersCount = 3;
				cfg.repeatedReads = true;
				emitBuffersGuard = newSwapBufferGuard(cfg);
			}

			void initialize() // opengl thread
			{
				renderQueue = newRenderQueue();
				provisionalData = newProvisionalGraphics();
				for (EmitBuffer &it : emitBuffers)
				{
					RenderPipelineCreateConfig cfg;
					cfg.assets = engineAssets();
					cfg.provisionalGraphics = +provisionalData;
					cfg.scene = +it.scene;
					it.pipeline = newRenderPipeline(cfg);
				}
			}

			void finalize() // opengl thread
			{
				for (EmitBuffer &it : emitBuffers)
					it.pipeline.clear();
				renderQueue.clear();
				provisionalData.clear();
			}

			void emit(uint64 emitTime) // control thread
			{
				if (auto lock = emitBuffersGuard->write())
				{
					EntitiesCopyConfig cfg;
					cfg.source = engineEntities();
					cfg.destination = +emitBuffers[lock.index()].scene;
					entitiesCopy(cfg);
					emitBuffers[lock.index()].emitTime = emitTime;
				}
			}

			void prepare(uint64 dispatchTime) // prepare thread
			{
				if (auto lock = emitBuffersGuard->read())
				{
					const EmitBuffer &eb = emitBuffers[lock.index()];
					renderQueue->resetQueue();
					eb.pipeline->windowResolution = engineWindow()->resolution();
					eb.pipeline->time = itc(eb.emitTime, dispatchTime, controlThread().updatePeriod());
					eb.pipeline->interpolationFactor = saturate(Real(eb.pipeline->time - eb.emitTime) / controlThread().updatePeriod());
					eb.pipeline->frameIndex = frameIndex;
					renderQueue->enqueue(eb.pipeline->render());
					renderQueue->resetFrameBuffer();
					renderQueue->resetAllTextures();
					renderQueue->resetAllState();
					outputDrawCalls = renderQueue->drawsCount();
					outputDrawPrimitives = renderQueue->primitivesCount();
					frameIndex++;
				}
			}

			void dispatch() // opengl thread
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

			void swap() // opengl thread
			{
				CAGE_CHECK_GL_ERROR_DEBUG();
				engineWindow()->swapBuffers();
				glFinish(); // this is where the engine should be waiting for the gpu
			}

			Holder<RenderQueue> renderQueue;
			Holder<ProvisionalGraphics> provisionalData;

			Holder<SwapBufferGuard> emitBuffersGuard;
			EmitBuffer emitBuffers[3];
			InterpolationTimingCorrector itc;

			uint32 outputDrawCalls = 0;
			uint32 outputDrawPrimitives = 0;
			uint32 frameIndex = 0;
		};

		Graphics *graphics;
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

	void graphicsEmit(uint64 emitTime)
	{
		graphics->emit(emitTime);
	}

	void graphicsPrepare(uint64 dispatchTime, uint32 &drawCalls, uint32 &drawPrimitives)
	{
		graphics->prepare(dispatchTime);
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
}
