#include <algorithm>
#include <array>
#include <vector>

#include "engine.h"
#include "interpolationTimingCorrector.h"

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/hashString.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/tasks.h>
#include <cage-engine/frameBuffer.h>
#include <cage-engine/graphicsError.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h>
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/renderPipeline.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneScreenSpaceEffects.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/window.h>

namespace cage
{
	namespace
	{
		const ConfigFloat confRenderGamma("cage/graphics/gamma", 2.2);

		struct TimeQuery : Noncopyable
		{
		private:
			uint32 id = 0;

		public:
			uint64 time = 0; // nanoseconds

			void start()
			{
				if (id == 0)
					glGenQueries(1, &id);
				else
					glGetQueryObjectui64v(id, GL_QUERY_RESULT, &time);
				glBeginQuery(GL_TIME_ELAPSED, id);
			}

			void finish()
			{
				if (id != 0)
					glEndQuery(GL_TIME_ELAPSED);
			}
		};

		struct EmitBuffer : private Immovable
		{
			Holder<EntityManager> scene = newEntityManager({ .linearAllocators = true });
			uint64 emitTime = 0;
		};

		struct CameraData : RenderPipelineConfig
		{
			Holder<RenderQueue> renderQueue;
			bool finalProduct = false; // ensure render-to-texture before render-to-window

			void operator()() { renderQueue = renderPipeline(*this); }

			bool operator<(const CameraData &other) const { return finalProduct < other.finalProduct; }
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
					const Vec2 os = data.orthographicSize * 0.5;
					return orthographicProjection(-os[0], os[0], -os[1], os[1], data.near, data.far);
				}
				case CameraTypeEnum::Perspective:
					return perspectiveProjection(data.perspectiveFov, Real(resolution[0]) / Real(resolution[1]), data.near, data.far);
				default:
					CAGE_THROW_ERROR(Exception, "invalid camera type");
			}
		}

		/*
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
					res.screenSize = data.orthographicSize[1] * screenHeight;
					res.orthographic = true;
					break;
				}
				case CameraTypeEnum::Perspective:
					res.screenSize = perspectiveScreenSize(data.perspectiveFov, screenHeight);
					break;
				default:
					CAGE_THROW_ERROR(Exception, "invalid camera type");
			}
			return res;
		}
		*/

		TextureHandle initializeTarget(const String &prefix, Vec2i resolution)
		{
			const String name = Stringizer() + prefix + "_" + resolution;
			TextureHandle tex = engineProvisionalGraphics()->texture(name,
				[resolution](Texture *tex)
				{
					tex->initialize(resolution, 1, GL_RGB8);
					tex->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					tex->filters(GL_LINEAR, GL_LINEAR, 0);
				});
			return tex;
		}

		Entity *findVrOrigin(EntityManager *scene)
		{
			auto r = scene->component<VrOriginComponent>()->entities();
			if (r.size() != 1)
				CAGE_THROW_ERROR(Exception, "there must be exactly one entity with VrOriginComponent");
			return r[0];
		}

		Transform transformByVrOrigin(EntityManager *scene, const Transform &in, Real interpolationFactor)
		{
			Entity *e = findVrOrigin(scene);
			const Transform t = modelTransform(e, interpolationFactor);
			const Transform &c = e->value<VrOriginComponent>().manualCorrection;
			return t * c * in;
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
				renderQueue = newRenderQueue("engine", engineProvisionalGraphics());
				onDemand = newAssetsOnDemand(engineAssets());
			}

			void finalize() // opengl thread
			{
				onDemand->clear(); // make sure to release all assets, but keep the structure to allow remaining calls to enginePurgeAssetsOnDemandCache
				renderQueue.clear();
				engineProvisionalGraphics()->purge();
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

			Vec2i applyDynamicResolution(Vec2i in) const
			{
				Vec2i res = Vec2i(Vec2(in) * dynamicResolution);
				CAGE_ASSERT(res[0] > 0 && res[1] > 0);
				return res;
			}

			void updateDynamicResolution()
			{
				if (frameIndex < nextAllowedDrFrameIndex)
					return;

				if (!engineDynamicResolution().enabled)
				{
					dynamicResolution = 1;
					return;
				}

				CAGE_ASSERT(engineDynamicResolution().targetFps > 0);
				CAGE_ASSERT(valid(engineDynamicResolution().minimumScale));
				CAGE_ASSERT(engineDynamicResolution().minimumScale > 0 && engineDynamicResolution().minimumScale <= 1);

				const double targetTime = 1'000'000'000 / engineDynamicResolution().targetFps;
				const double avgTime = (timeQueries[0].time + timeQueries[1].time + timeQueries[2].time) / 3;
				Real k = dynamicResolution * targetTime / avgTime;
				if (!valid(k))
					return;
				k = min(k, dynamicResolution + 0.03); // progressive restoration
				if (k > 0.97)
					k = 1; // snap back to 100 %
				k = clamp(k, engineDynamicResolution().minimumScale, 1); // safety clamp
				if (abs(dynamicResolution - k) < 0.02)
					return; // difference of at least 2 percents

				dynamicResolution = k;
				nextAllowedDrFrameIndex = frameIndex + 5;
			}

			void prepareCameras(const RenderPipelineConfig &cfg, Holder<VirtualRealityGraphicsFrame> vrFrame)
			{
				std::vector<CameraData> cameras;
				std::vector<TextureHandle> vrTargets;
				TextureHandle windowTarget;
				const bool enabled = cfg.resolution[0] > 0 && cfg.resolution[1] > 0;

				entitiesVisitor(
					[&](Entity *e, const CameraComponent &cam)
					{
						if (!enabled)
						{
							if (!cam.target)
								return; // no rendering into minimized window
							if (!vrFrame)
								return; // no intermediate renders are used
						}
						CameraData data;
						(RenderPipelineConfig &)data = cfg;
						data.camera = cam;
						data.cameraSceneMask = e->getOrDefault<SceneComponent>().sceneMask;
						data.effects = e->getOrDefault<ScreenSpaceEffectsComponent>();
						if (dynamicResolution != 1)
							data.effects.effects &= ~ScreenSpaceEffectsFlags::AntiAliasing;
						data.effects.gamma = Real(confRenderGamma);
						data.name = Stringizer() + "camera_" + e->id();
						data.resolution = cam.target ? cam.target->resolution() : applyDynamicResolution(cfg.resolution);
						data.transform = modelTransform(e, cfg.interpolationFactor);
						data.projection = initializeProjection(cam, data.resolution);
						data.lodSelection = LodSelection(data.transform.position, cam, data.resolution[1]);
						if (cam.target)
							data.target = TextureHandle(Holder<Texture>(cam.target, nullptr));
						else
						{
							CAGE_ASSERT(!windowTarget);
							windowTarget = initializeTarget(Stringizer() + "windowTarget_" + data.name, data.resolution);
							data.target = windowTarget;
						}
						data.finalProduct = !cam.target;
						cameras.push_back(std::move(data));
					},
					cfg.scene, false);

				if (vrFrame)
				{
					Entity *camEnt = nullptr;
					{
						auto r = cfg.scene->component<VrCameraComponent>()->entities();
						if (!r.empty())
						{
							camEnt = r[0];
							const auto &cam = camEnt->value<VrCameraComponent>();
							for (VirtualRealityCamera &it : vrFrame->cameras)
							{
								it.nearPlane = cam.near;
								it.farPlane = cam.far;
							}
						}
					}

					vrFrame->updateProjections();

					uint32 index = 0;
					for (const VirtualRealityCamera &it : vrFrame->cameras)
					{
						CameraData data;
						(RenderPipelineConfig &)data = cfg;
						if (camEnt)
						{
							data.camera = camEnt->value<VrCameraComponent>();
							if (camEnt->has<ScreenSpaceEffectsComponent>())
								data.effects = camEnt->value<ScreenSpaceEffectsComponent>();
						}
						if (dynamicResolution != 1)
							data.effects.effects &= ~ScreenSpaceEffectsFlags::AntiAliasing;
						data.effects.gamma = Real(confRenderGamma);
						data.name = Stringizer() + "vrCamera_" + index;
						data.resolution = applyDynamicResolution(it.resolution);
						data.transform = transformByVrOrigin(cfg.scene, it.transform, cfg.interpolationFactor);
						data.projection = it.projection;
						data.lodSelection = LodSelection(it.primary ? vrFrame->pose().position : it.transform.position, CameraComponent{ .perspectiveFov = it.verticalFov }, data.resolution[1]);
						data.target = initializeTarget(Stringizer() + "vrTarget_" + data.name, data.resolution);
						data.finalProduct = true;
						vrTargets.push_back(data.target);
						cameras.push_back(std::move(data));
						index++;
					}
				}

				std::stable_sort(cameras.begin(), cameras.end()); // ensure render-to-texture before render-to-window
				tasksRunBlocking<CameraData>("prepare camera task", cameras);

				if (vrFrame)
				{
					renderQueue->customCommand(vrFrame.share(), +[](VirtualRealityGraphicsFrame *f) { f->renderBegin(); });
				}

				for (CameraData &cam : cameras)
					if (cam.renderQueue)
						renderQueue->enqueue(std::move(cam.renderQueue));

				Holder<ShaderProgram> shaderBlit = cfg.assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/engine/blitScaled.glsl"))->get(0);
				CAGE_ASSERT(shaderBlit);
				renderQueue->bind(shaderBlit);

				// blit to window
				if (enabled)
				{
					renderQueue->resetFrameBuffer();
					renderQueue->viewport(Vec2i(), cfg.resolution);
					if (windowTarget)
					{
						renderQueue->bind(windowTarget, 0);
						Holder<Model> modelSquare = cfg.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));
						CAGE_ASSERT(modelSquare);
						renderQueue->draw(modelSquare);
					}
					else
					{
						// clear window (we have no cameras rendering into it)
						renderQueue->clear(true, false);
					}
				}

				// blit to vr targets
				if (vrFrame)
				{
					struct VrBlit
					{
						Holder<VirtualRealityGraphicsFrame> frame;
						std::vector<TextureHandle> targets;

						void operator()()
						{
							Holder<Model> modelSquare = engineAssets()->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));
							CAGE_ASSERT(modelSquare);
							modelSquare->bind();
							Holder<FrameBuffer> renderTarget = engineProvisionalGraphics()->frameBufferDraw("vr_blit")->resolve();
							renderTarget->bind();
							frame->acquireTextures();
							CAGE_ASSERT(frame->cameras.size() == targets.size());
							for (uint32 i = 0; i < targets.size(); i++)
							{
								if (!frame->cameras[i].colorTexture)
									continue;
								renderTarget->colorTexture(0, frame->cameras[i].colorTexture);
								renderTarget->checkStatus();
								glViewport(0, 0, frame->cameras[i].resolution[0], frame->cameras[i].resolution[1]);
								targets[i].resolve()->bind(0);
								modelSquare->dispatch();
								CAGE_CHECK_GL_ERROR_DEBUG();
							}
							frame->renderCommit();
						}
					};

					Holder<VrBlit> vrBlit = systemMemory().createHolder<VrBlit>();
					vrBlit->frame = std::move(vrFrame);
					vrBlit->targets = std::move(vrTargets);
					renderQueue->customCommand(std::move(vrBlit));
				}

				// reset the universe
				renderQueue->resetFrameBuffer();
				renderQueue->resetAllTextures();
				renderQueue->resetAllState();
			}

			void prepare(uint64 dispatchTime, uint32 &drawCalls, uint32 &drawPrimitives) // prepare thread
			{
				if (auto lock = emitBuffersGuard->read())
				{
					renderQueue->resetQueue();

					Holder<VirtualRealityGraphicsFrame> vrFrame;
					if (engineVirtualReality())
					{
						vrFrame = engineVirtualReality()->graphicsFrame();
						dispatchTime = vrFrame->displayTime();
					}

					updateDynamicResolution();
					const EmitBuffer &eb = emitBuffers[lock.index()];
					const Vec2i windowResolution = engineWindow()->resolution();
					RenderPipelineConfig cfg;
					cfg.assets = engineAssets();
					cfg.provisionalGraphics = engineProvisionalGraphics();
					cfg.scene = +eb.scene;
					cfg.onDemand = +onDemand;
					if (!graphicsPrepareThread().disableTimePassage)
					{
						const uint64 period = controlThread().updatePeriod();
						cfg.currentTime = itc(eb.emitTime, dispatchTime, period);
						cfg.elapsedTime = dispatchTime - lastDispatchTime;
						cfg.interpolationFactor = saturate(Real(cfg.currentTime - eb.emitTime) / period);
						cfg.frameIndex = frameIndex;
					}
					cfg.resolution = windowResolution;

					if (cfg.assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) && cfg.assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shaders/engine/engine.pack")))
						prepareCameras(cfg, vrFrame.share());
					else if (vrFrame)
					{
						renderQueue->customCommand(
							vrFrame.share(),
							+[](VirtualRealityGraphicsFrame *f)
							{
								f->renderBegin();
								f->renderCancel();
							});
					}

					onDemand->process();
					drawCalls = renderQueue->drawsCount();
					drawPrimitives = renderQueue->primitivesCount();
					frameIndex++;
					lastDispatchTime = dispatchTime;
				}
				else
				{
					drawCalls = drawPrimitives = 0;
				}
			}

			void dispatch() // opengl thread
			{
				renderQueue->dispatch();

				{
					const auto _ = ProfilingScope("reset provisionals");
					engineProvisionalGraphics()->reset();
				}

				// check gl errors (even in release, but do not halt the game)
				try
				{
					checkGlError();
				}
				catch (const GraphicsError &)
				{
					// nothing
				}
			}

			void swap() // opengl thread
			{
				CAGE_CHECK_GL_ERROR_DEBUG();
				engineWindow()->swapBuffers();
			}

			void frameStart() { timeQueries[0].start(); }

			void frameFinish()
			{
				timeQueries[0].finish();
				std::rotate(timeQueries.begin(), timeQueries.begin() + 1, timeQueries.end());
			}

			Holder<RenderQueue> renderQueue;
			Holder<AssetsOnDemand> onDemand;

			Holder<SwapBufferGuard> emitBuffersGuard;
			EmitBuffer emitBuffers[3];
			InterpolationTimingCorrector itc;

			uint64 lastDispatchTime = 0;
			uint32 frameIndex = 0;
			Real dynamicResolution = 1;
			uint32 nextAllowedDrFrameIndex = 100; // do not update DR at the very start
			std::array<TimeQuery, 3> timeQueries = {};
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

	void graphicsPrepare(uint64 dispatchTime, uint32 &drawCalls, uint32 &drawPrimitives, Real &dynamicResolution)
	{
		graphics->prepare(dispatchTime, drawCalls, drawPrimitives);
		dynamicResolution = graphics->dynamicResolution;
	}

	void graphicsDispatch()
	{
		graphics->dispatch();
	}

	void graphicsSwap()
	{
		graphics->swap();
	}

	void graphicsFrameStart()
	{
		graphics->frameStart();
	}

	void graphicsFrameFinish(uint64 &gpuTime)
	{
		graphics->frameFinish();
		gpuTime = graphics->timeQueries[0].time / 1000;
	}

	AssetsOnDemand *engineAssetsOnDemand()
	{
		CAGE_ASSERT(graphics && graphics->onDemand);
		return +graphics->onDemand;
	}
}
