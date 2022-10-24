#include <cage-core/swapBufferGuard.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/assetManager.h>
#include <cage-core/entitiesCopy.h>
#include <cage-core/hashString.h>
#include <cage-core/entities.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/tasks.h>

#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderPipeline.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/model.h>

#include "engine.h"
#include "interpolationTimingCorrector.h"

#include <vector>

namespace cage
{
	namespace
	{
		const ConfigSint32 confVisualizeBuffer("cage/graphics/visualizeBuffer", 0);
		const ConfigFloat confRenderGamma("cage/graphics/gamma", 2.2);

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
			bool order = false;

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

		Mat4 initializeProjection(const CameraComponent &data, const Vec2i resolution)
		{
			switch (data.cameraType)
			{
			case CameraTypeEnum::Orthographic:
			{
				const Vec2 &os = data.camera.orthographicSize;
				return orthographicProjection(-os[0], os[0], -os[1], os[1], data.near, data.far);
			}
			case CameraTypeEnum::Perspective: return perspectiveProjection(data.camera.perspectiveFov, Real(resolution[0]) / Real(resolution[1]), data.near, data.far);
			default: CAGE_THROW_ERROR(Exception, "invalid camera type");
			}
		}

		Real perspectiveScreenSize(Rads vFov, sint32 screenHeight)
		{
			return tan(vFov * 0.5) * 2 * screenHeight;
		}

		LodSelection initializeLodSelection(const CameraComponent &data, sint32 screenHeight)
		{
			LodSelection res;
			switch (data.cameraType)
			{
			case CameraTypeEnum::Orthographic:
			{
				res.screenSize = data.camera.orthographicSize[1] * screenHeight;
				res.orthographic = true;
			} break;
			case CameraTypeEnum::Perspective:
				res.screenSize = perspectiveScreenSize(data.camera.perspectiveFov, screenHeight);
				break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid camera type");
			}
			return res;
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
				provisionalData = newProvisionalGraphics();
				renderQueue = newRenderQueue("engine", +provisionalData);
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

				if (auto vr = engineVirtualReality())
				{
					vrFrame = vr->graphicsFrame();

					Entity *origEnt = nullptr;
					{
						auto r = eb.scene->component<VrOriginComponent>()->entities();
						if (!r.empty())
							origEnt = r[0];
					}
					Entity *camEnt = nullptr;
					{
						auto r = eb.scene->component<VrCameraComponent>()->entities();
						if (!r.empty())
							camEnt = r[0];
					}

					if (camEnt)
					{
						const auto &cam = camEnt->value<VrCameraComponent>();
						for (VirtualRealityCamera &it : vrFrame->cameras)
						{
							it.nearPlane = cam.near;
							it.farPlane = cam.far;
						}
					}

					vrFrame->updateProjections();
					vrTargets.clear();

					uint32 index = 0;
					for (const VirtualRealityCamera &it : vrFrame->cameras)
					{
						CameraData data;
						data.pipeline = eb.pipeline.share();
						if (camEnt)
						{
							data.inputs.camera = camEnt->value<VrCameraComponent>();
							if (camEnt->has<ScreenSpaceEffectsComponent>())
								data.inputs.effects = camEnt->value<ScreenSpaceEffectsComponent>();
						}
						data.inputs.effects.gamma = Real(confRenderGamma);
						data.inputs.name = Stringizer() + "vrcamera_" + index;
						data.inputs.target = initializeVrTarget(data.inputs.name, it.resolution);
						vrTargets.push_back(data.inputs.target);
						data.inputs.resolution = it.resolution;
						data.inputs.transform = it.transform; // todo transform relative to the origin
						data.inputs.projection = it.projection;
						data.inputs.lodSelection.screenSize = perspectiveScreenSize(it.verticalFov, data.inputs.resolution[1]);
						data.inputs.lodSelection.center = vrFrame->pose().position;
						cameras.push_back(std::move(data));
						index++;
					}
				}

				uint32 windowOutputs = 0;
				entitiesVisitor([&](Entity *e, const CameraComponent &cam) {
					CameraData data;
					data.pipeline = eb.pipeline.share();
					data.inputs.camera = cam;
					if (e->has<ScreenSpaceEffectsComponent>())
						data.inputs.effects = e->value<ScreenSpaceEffectsComponent>();
					data.inputs.effects.gamma = Real(confRenderGamma);
					data.inputs.name = Stringizer() + "camera_" + e->name();
					data.inputs.target = cam.target ? TextureHandle(Holder<Texture>(cam.target, nullptr)) : TextureHandle();
					data.inputs.resolution = cam.target ? cam.target->resolution() : windowResolution;
					data.inputs.transform = modelTransform(e, eb.pipeline->interpolationFactor);
					data.inputs.projection = initializeProjection(cam, data.inputs.resolution);
					data.inputs.lodSelection = initializeLodSelection(cam, data.inputs.resolution[1]);
					data.inputs.lodSelection.center = data.inputs.transform.position;
					data.order = !cam.target;
					cameras.push_back(std::move(data));
					windowOutputs += cam.target ? 0 : 1;
				}, +eb.scene, false);
				CAGE_ASSERT(windowOutputs <= 1);

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
						deb.shader = engineAssets()->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/blit.glsl"))->get(0);
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
					renderQueue->bind(debugVisualizations[index].texture, 0);
					renderQueue->bind(debugVisualizations[index].shader);
					renderQueue->uniform(debugVisualizations[index].shader, 0, 1.0 / Vec2(windowResolution));
					renderQueue->draw(engineAssets()->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj")));
					renderQueue->checkGlErrorDebug();
				}
			}

			void prepare(uint64 dispatchTime) // prepare thread
			{
				if (auto lock = emitBuffersGuard->read())
				{
					const EmitBuffer &eb = emitBuffers[lock.index()];
					eb.pipeline->currentTime = itc(eb.emitTime, dispatchTime, controlThread().updatePeriod());
					eb.pipeline->elapsedTime = dispatchTime - lastDispatchTime;
					eb.pipeline->interpolationFactor = saturate(Real(eb.pipeline->currentTime - eb.emitTime) / controlThread().updatePeriod());
					eb.pipeline->frameIndex = frameIndex;

					renderQueue->resetQueue();
					prepare(eb);
					renderQueue->resetFrameBuffer();
					renderQueue->resetAllTextures();
					renderQueue->resetAllState();

					outputDrawCalls = renderQueue->drawsCount();
					outputDrawPrimitives = renderQueue->primitivesCount();
					frameIndex++;
					lastDispatchTime = dispatchTime;
				}
			}

			void dispatch() // opengl thread
			{
				if (vrFrame)
				{
					vrFrame->renderBegin();
					copyVrContents();
					renderQueue->dispatch();
					vrFrame->renderCommit();
				}
				else
					renderQueue->dispatch();

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

			TextureHandle initializeVrTarget(const String &prefix, Vec2i resolution)
			{
				auto q = +renderQueue;
				const String name = Stringizer() + prefix + "_" + resolution;
				TextureHandle tex = provisionalData->texture(name);
				if (tex.first())
				{
					q->image2d(tex, resolution, 1, GL_SRGB8_ALPHA8);
					q->wraps(tex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					q->filters(tex, GL_LINEAR, GL_LINEAR, 0);
				}
				return tex;
			}

			void copyVrContents()
			{
				const auto graphicsDebugScope = renderQueue->namedScope("VR blit");
				CAGE_ASSERT(vrFrame);
				CAGE_ASSERT(vrFrame->cameras.size() == vrTargets.size());
				
				auto assets = engineAssets();
				Holder<Model> modelSquare = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				Holder<ShaderProgram> shaderBlit = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/blit.glsl"))->get(0);
				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("VR_blit");

				renderQueue->bind(shaderBlit);
				renderQueue->bind(renderTarget);
				for (uint32 i = 0; i < vrTargets.size(); i++)
				{
					if (!vrFrame->cameras[i].colorTexture)
						continue;
					renderQueue->colorTexture(renderTarget, 0, TextureHandle(Holder<Texture>(vrFrame->cameras[i].colorTexture, nullptr)));
					renderQueue->checkFrameBuffer(renderTarget);
					renderQueue->viewport(Vec2i(), vrFrame->cameras[i].resolution);
					renderQueue->bind(vrTargets[i], 0);
					renderQueue->draw(modelSquare);
				}
				renderQueue->resetFrameBuffer();
			}

			Holder<RenderQueue> renderQueue;
			Holder<ProvisionalGraphics> provisionalData;

			Holder<VirtualRealityGraphicsFrame> vrFrame;
			std::vector<TextureHandle> vrTargets;

			Holder<SwapBufferGuard> emitBuffersGuard;
			EmitBuffer emitBuffers[3];
			InterpolationTimingCorrector itc;

			uint64 lastDispatchTime = 0;
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
