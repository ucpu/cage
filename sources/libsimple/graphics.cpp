#include <cage-core/swapBufferGuard.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/assetManager.h>
#include <cage-core/entitiesCopy.h>
#include <cage-core/hashString.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/tasks.h>

#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderPipeline.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h>
#include <cage-engine/scene.h>
#include <cage-engine/model.h>

#include "engine.h"
#include "interpolationTimingCorrector.h"

#include <vector>

namespace cage
{
	namespace
	{
		ConfigSint32 confVisualizeBuffer("cage/graphics/visualizeBuffer", 0);

		struct EmitBuffer : private Immovable
		{
			Holder<RenderPipeline> pipeline;
			Holder<EntityManager> scene = newEntityManager();
			uint64 emitTime = 0;
		};

		struct CameraData
		{
			Holder<RenderPipeline> pipeline;
			RenderPipelineCamera inputs;
			RenderPipelineResult outputs;
			std::pair<bool, sint32> order;

			void operator() ()
			{
				outputs = pipeline->render(inputs);
			}

			bool operator < (const CameraData &other) const
			{
				return order < other.order;
			}
		};

		Transform modelTransform(Entity *e, Real interpolationFactor)
		{
			EntityComponent *transformComponent = e->manager()->component<TransformComponent>();
			EntityComponent *prevTransformComponent = e->manager()->componentsByType(detail::typeIndex<TransformComponent>())[1];
			CAGE_ASSERT(e->has(transformComponent));
			if (e->has(prevTransformComponent))
			{
				const Transform c = e->value<TransformComponent>(transformComponent);
				const Transform p = e->value<TransformComponent>(prevTransformComponent);
				return interpolate(p, c, interpolationFactor);
			}
			else
				return e->value<TransformComponent>(transformComponent);
		}

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

			void prepare(const EmitBuffer &eb)
			{
				const Vec2i windowResolution = engineWindow()->resolution();
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;

				if (!eb.pipeline->reinitialize())
					return;

				std::vector<CameraData> cameras;
				uint32 windowOutputs = 0;
				entitiesVisitor([&](Entity *e, const CameraComponent &cam) {
					CameraData data;
					data.pipeline = eb.pipeline.share();
					data.inputs.camera = cam;
					data.inputs.name = Stringizer() + "camera_" + e->name();
					data.inputs.target = cam.target ? TextureHandle(Holder<Texture>(cam.target, nullptr)) : TextureHandle();
					data.inputs.resolution = cam.target ? cam.target->resolution() : windowResolution;
					data.inputs.transform = modelTransform(e, eb.pipeline->interpolationFactor);
					data.order = std::make_pair(!cam.target, cam.cameraOrder);
					cameras.push_back(std::move(data));
					windowOutputs += cam.target ? 0 : 1;
				}, +eb.scene, false);
				CAGE_ASSERT(windowOutputs == 1);
				if (cameras.empty())
					return;

				std::sort(cameras.begin(), cameras.end());
				tasksRunBlocking<CameraData>("prepare camera task", cameras);

				std::vector<RenderPipelineDebugVisualization> debugVisualizations;
				for (CameraData &cam : cameras)
				{
					renderQueue->enqueue(std::move(cam.outputs.renderQueue));
					if (cam.inputs.target)
					{
						RenderPipelineDebugVisualization deb;
						deb.texture = cam.inputs.target;
						deb.shader = engineAssets()->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/blit.glsl"));
						CAGE_ASSERT(deb.shader);
						debugVisualizations.push_back(std::move(deb));
					}
					for (RenderPipelineDebugVisualization &di : cam.outputs.debugVisualizations)
					{
						CAGE_ASSERT(di.shader);
						debugVisualizations.push_back(std::move(di));
					}
				}

				const uint32 cnt = numeric_cast<uint32>(debugVisualizations.size()) + 1;
				const uint32 index = (confVisualizeBuffer % cnt + cnt) % cnt - 1;
				if (index != m)
				{
					CAGE_ASSERT(index < debugVisualizations.size());
					const auto graphicsDebugScope = renderQueue->namedScope("visualize buffer");
					renderQueue->viewport(Vec2i(), windowResolution);
					renderQueue->bind(engineAssets()->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
					renderQueue->bind(debugVisualizations[index].texture, 0);
					renderQueue->bind(debugVisualizations[index].shader);
					renderQueue->uniform(0, 1.0 / Vec2(windowResolution));
					renderQueue->draw();
					renderQueue->checkGlErrorDebug();
				}
			}

			void prepare(uint64 dispatchTime) // prepare thread
			{
				if (auto lock = emitBuffersGuard->read())
				{
					const EmitBuffer &eb = emitBuffers[lock.index()];
					eb.pipeline->time = itc(eb.emitTime, dispatchTime, controlThread().updatePeriod());
					eb.pipeline->interpolationFactor = saturate(Real(eb.pipeline->time - eb.emitTime) / controlThread().updatePeriod());
					eb.pipeline->frameIndex = frameIndex;

					renderQueue->resetQueue();
					prepare(eb);
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
