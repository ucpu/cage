#include <algorithm>
#include <array>
#include <atomic>
#include <optional>
#include <vector>

#include "engine.h"

#include <cage-core/assetsManager.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/hashString.h>
#include <cage-core/image.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/tasks.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/guiManager.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneRender.h>
#include <cage-engine/sceneVirtualReality.h>
#include <cage-engine/texture.h>
#include <cage-engine/virtualReality.h>
#include <cage-engine/window.h>

namespace cage
{
	struct AssetPack;

	namespace
	{
		const ConfigFloat confRenderGamma("cage/graphics/gamma", 1.0);

		struct InterpolationTimingCorrector
		{
			uint64 operator()(uint64 emit, uint64 dispatch, uint64 step)
			{
				CAGE_ASSERT(step > 0);
				corrections.add((sint64)emit - (sint64)dispatch);
				const sint64 c = corrections.smooth();
				return max(emit, dispatch + c + step / 2);
			}

			VariableSmoothingBuffer<sint64, 60> corrections;
		};

		struct EmitBuffer : private Immovable
		{
			Holder<EntityManager> entities;
			uint64 emitTime = 0;
		};

		Transform modelTransform(Entity *e, Real interpolationFactor)
		{
			EntityComponent *curr = e->manager()->component<TransformComponent>();
			EntityComponent *prev = e->manager()->componentsByType(detail::typeIndex<TransformComponent>())[1];
			CAGE_ASSERT(e->has(curr));
			Transform c = e->value<TransformComponent>(curr);
			if (e->has(prev))
			{
				const Transform p = e->value<TransformComponent>(prev);
				c = interpolate(p, c, interpolationFactor);
			}
			return c;
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
			}
			CAGE_THROW_ERROR(Exception, "invalid camera type");
		}

		Transform transformByVrOrigin(EntityManager *scene, const Transform &in, Real interpolationFactor)
		{
			Entity *e = virtualRealityFindOrigin(scene);
			const Transform t = modelTransform(e, interpolationFactor);
			const Transform &c = e->value<VrOriginComponent>().manualCorrection;
			return t * c * in;
		}

		Vec2i updateResolution(Vec2i in, Real factor)
		{
			CAGE_ASSERT(factor.valid() && factor > 0);
			Vec2i res = Vec2i(Vec2(in) * factor);
			CAGE_ASSERT(res[0] > 0 && res[1] > 0);
			return res;
		}

		enum class ScreenshotStateEnum
		{
			None,
			Request,
			Copying,
			Failed,
		};

		struct ScreenshotData
		{
			gpu::Buffer readbackBuffer;
			Vec2i resolution;
		};

		class EnginePrivateGraphicsImpl : public EnginePrivateGraphics
		{
			Holder<SwapBufferGuard> emitBuffersGuard;
			std::array<EmitBuffer, 3> emitBuffers;
			InterpolationTimingCorrector itc;
			Holder<Texture> windowTexture;
			std::atomic<ScreenshotStateEnum> scrnshtState = ScreenshotStateEnum::None;
			std::optional<ScreenshotData> scrnshtData;

			uint64 lastDispatchTime = 0;
			uint32 frameIndex = 0;

			std::array<uint64, 3> gpuTimes = {};
			uint32 nextAllowedDrFrameIndex = 100; // do not update DR at the very start

		public:
			// control thread ---------------------------------------------------------------------

			EnginePrivateGraphicsImpl(const EngineCreateConfig &config)
			{
				SwapBufferGuardCreateConfig cfg;
				cfg.buffersCount = 3;
				cfg.repeatedReads = true;
				emitBuffersGuard = newSwapBufferGuard(cfg);
			}

			void emit(uint64 emitTime)
			{
				if (auto lock = emitBuffersGuard->write())
				{
					ProfilingScope profiling("copying entities");
					EmitBuffer &eb = emitBuffers[lock.index()];
					if (!eb.entities)
						eb.entities = newEntityManager({ .linearAllocators = true });
					EntitiesCopyConfig cfg;
					cfg.source = engineEntities();
					cfg.destination = +eb.entities;
					entitiesCopy(cfg);
					eb.emitTime = emitTime;
				}
			}

			Holder<Image> screenshot()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "requesting to make a screenshot");

				ScopeGuard scopeExit(
					[this]()
					{
						scrnshtState = ScreenshotStateEnum::None;
						scrnshtData.reset();
					});

				CAGE_ASSERT(scrnshtState == ScreenshotStateEnum::None);
				scrnshtState = ScreenshotStateEnum::Request;
				for (uint32 i = 0; i < 1'000; i++)
				{
					threadSleep(5'000);
					switch (scrnshtState)
					{
						case ScreenshotStateEnum::None:
							CAGE_ASSERT(!"ScreenshotStateEnum::None");
							return {};
						case ScreenshotStateEnum::Request:
							continue; // keep waiting
						case ScreenshotStateEnum::Copying:
							break; // screenshot ready for copying
						case ScreenshotStateEnum::Failed:
							CAGE_THROW_ERROR(Exception, "screenshot failed (window minimized?)");
					}
				}
				if (scrnshtState != ScreenshotStateEnum::Copying)
					CAGE_THROW_ERROR(Exception, "timed out waiting for screenshot");

				// wait for the copying on the device to finish
				engineGraphicsDevice()->nativeDevice()->waitDeviceIdle();

				CAGE_ASSERT(scrnshtData);
				Holder<Image> img = newImage();
				img->initialize(scrnshtData->resolution, 4);
				const auto data = scrnshtData->readbackBuffer.getMappedRange();
				detail::memcpy((void *)img->rawViewU8().data(), data.data(), data.size());

				// BGR -> RGB
				for (uint32 y = 0; y < scrnshtData->resolution[1]; y++)
				{
					for (uint32 x = 0; x < scrnshtData->resolution[0]; x++)
					{
						Vec4 c = img->get4(x, y);
						std::swap(c[0], c[2]);
						img->set(x, y, c);
					}
				}

				CAGE_LOG(SeverityEnum::Info, "graphics", "screenshot done");
				return img;
			}

			// graphics thread ---------------------------------------------------------------------

			void initialize() {}

			void finalize()
			{
				scrnshtData.reset();
				windowTexture.clear();
			}

			void updateDynamicResolution()
			{
				std::swap(gpuTimes[0], gpuTimes[1]);
				std::swap(gpuTimes[1], gpuTimes[2]);
				gpuTimes[2] = frameStatistics.gpuTime;

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

				const double targetTime = 1'000'000 / engineDynamicResolution().targetFps;
				const double avgTime = (gpuTimes[0] + gpuTimes[1] + gpuTimes[2]) / 3;
				Real k = dynamicResolution * targetTime / avgTime;
				if (!valid(k))
					return;
				k = min(k, dynamicResolution + 0.03); // progressive restoration
				if (k > 0.97)
					k = 1; // snap back to 100 %
				k = clamp(k, engineDynamicResolution().minimumScale, 1); // safety clamp
				if (abs(dynamicResolution - k) < 0.02)
					return; // difference of at least 2 percents

				CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "changing dynamic resolution from: " + dynamicResolution + ", to: " + k);
				dynamicResolution = k;
				nextAllowedDrFrameIndex = frameIndex + 5;
			}

			std::vector<SceneRenderCamera> generateCameras(const SceneRenderShared &cfg, VirtualRealityGraphicsFrame *vrFrame) const
			{
				std::vector<SceneRenderCamera> cameras;
				if (!cfg.scene)
					return cameras;

				cameras.reserve(cfg.scene->component<CameraComponent>()->count() + (vrFrame ? vrFrame->cameras.size() : 0));

				// standard cameras
				const auto &addCameras = [&](bool renderToTexture)
				{
					entitiesVisitor(
						[&](Entity *e, const CameraComponent &cam)
						{
							SceneRenderCamera data;
							if (renderToTexture)
								data.target = cam.target;
							else if (!cam.target)
								data.target = +windowTexture;
							if (!data.target)
								return;
							data.camera = cam;
							data.resolution = data.target->resolution();
							data.cameraSceneMask = e->getOrDefault<SceneComponent>().sceneMask;
							data.effects = e->getOrDefault<ScreenSpaceEffectsComponent>();
							data.effects.gamma = Real(confRenderGamma);
							if (cam.renderingResolution != 1)
							{
								data.resolution = updateResolution(data.resolution, cam.renderingResolution);
							}
							if (dynamicResolution != 1)
							{
								data.effects.effects &= ~ScreenSpaceEffectsFlags::AntiAliasing;
								data.resolution = updateResolution(data.resolution, dynamicResolution);
							}
							data.transform = modelTransform(e, cfg.interpolationFactor);
							data.projection = initializeProjection(cam, data.resolution);
							data.lodSelection = LodSelection(data.transform.position, cam, data.resolution[1]);
							cameras.push_back(std::move(data));
						},
						cfg.scene, false);
				};
				addCameras(true);
				addCameras(false);

				// virtual reality cameras
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

					for (const VirtualRealityCamera &it : vrFrame->cameras)
					{
						SceneRenderCamera data;
						data.target = it.colorTexture;
						data.resolution = it.resolution;
						if (camEnt)
						{
							data.camera = camEnt->value<VrCameraComponent>();
							data.cameraSceneMask = camEnt->getOrDefault<SceneComponent>().sceneMask;
							data.effects = camEnt->getOrDefault<ScreenSpaceEffectsComponent>();
						}
						data.effects.gamma = Real(confRenderGamma);
						if (dynamicResolution != 1)
						{
							data.effects.effects &= ~ScreenSpaceEffectsFlags::AntiAliasing;
							data.resolution = updateResolution(data.resolution, dynamicResolution);
						}
						data.transform = transformByVrOrigin(cfg.scene, it.transform, cfg.interpolationFactor);
						data.projection = it.projection;
						data.lodSelection = LodSelection(it.primary ? vrFrame->pose().position : it.transform.position, CameraComponent{ .perspectiveFov = it.verticalFov }, data.resolution[1]);
						cameras.push_back(std::move(data));
					}
				}

				return cameras;
			}

			void takeScreenshot(Texture *srcTex)
			{
				CAGE_ASSERT(scrnshtState == ScreenshotStateEnum::Request);
				const Vec2i resolution = srcTex->resolution();
				CAGE_ASSERT(resolution[0] > 0 && resolution[1] > 0);
				gpu::BufferDescriptor readbackDesc;
				readbackDesc.label = "screenshot readback";
				readbackDesc.usage = gpu::BufferUsageFlags::MapRead | gpu::BufferUsageFlags::CopyDst;
				readbackDesc.size = resolution[0] * resolution[1] * 4;
				gpu::Buffer readbackBuffer = engineGraphicsDevice()->nativeDevice()->createBuffer(readbackDesc);
				gpu::TexelCopyTextureInfo srcView;
				srcView.texture = srcTex->nativeTexture();
				gpu::CommandEncoder encoder = engineGraphicsDevice()->nativeDevice()->createCommandEncoder({ .label = "screenshot copy" });
				encoder.copyTextureToBuffer(srcView, readbackBuffer, 0, Vec3i(resolution, 1));
				gpu::CommandBuffer cmd = encoder.finishEncoding();
				engineGraphicsDevice()->insertCommandBuffer(std::move(cmd), {});
				scrnshtData = ScreenshotData{ std::move(readbackBuffer), resolution };
				scrnshtState = ScreenshotStateEnum::Copying;
			}

			void dispatch(uint64 dispatchTime, Holder<GuiRender> guiBundle)
			{
				ScopeGuard scopeExit([this]() { windowTexture.clear(); });

				GraphicsWindowPresentation gwp;
				gwp.window = engineWindow();
				frameStatistics = engineGraphicsDevice()->nextFrame(PointerRange(gwp));
				windowTexture = std::move(gwp.texture);
				updateDynamicResolution();

				Holder<VirtualRealityGraphicsFrame> vrFrame;
				if (engineVirtualReality())
				{
					vrFrame = engineVirtualReality()->nextFrame();
					if (vrFrame)
						dispatchTime = vrFrame->displayTime();
				}

				if ((!windowTexture && !vrFrame) || !engineAssets()->get<AssetPack>(HashString("cage/cage.pack")))
				{
					threadSleep(15'000); // prevent fast looping when the window is minimized
					if (scrnshtState == ScreenshotStateEnum::Request)
						scrnshtState = ScreenshotStateEnum::Failed;
					return;
				}

				auto lock = emitBuffersGuard->read();
				if (!lock)
					return;
				EmitBuffer &eb = emitBuffers[lock.index()];

				{
					ProfilingScope profiling("scene prepare & render");
					SceneRenderConfig cfg;
					cfg.shared.device = engineGraphicsDevice();
					cfg.shared.assets = engineAssets();
					cfg.shared.onDemand = engineAssetsOnDemand();
					cfg.shared.scene = +eb.entities;
					if (!graphicsThread().disableTimePassage)
					{
						const uint64 period = controlThread().updatePeriod();
						cfg.shared.currentTime = itc(eb.emitTime, dispatchTime, period);
						cfg.shared.elapsedTime = dispatchTime - lastDispatchTime;
						cfg.shared.interpolationFactor = saturate(Real(cfg.shared.currentTime - eb.emitTime) / period);
						cfg.shared.frameIndex = frameIndex;
					}
					std::vector<SceneRenderCamera> cameras = generateCameras(cfg.shared, +vrFrame);
					cfg.cameras = cameras;
					const auto commands = sceneRender(cfg);
					for (const auto &cmd : commands)
						cmd->submit();
				}

				if (windowTexture)
				{
					if (guiBundle)
					{
						ProfilingScope profiling("gui dispatch");
						Holder<GraphicsEncoder> enc = newGraphicsEncoder(engineGraphicsDevice(), "gui");
						Holder<GraphicsAggregateBuffer> agg = newGraphicsAggregateBuffer({ engineGraphicsDevice() });
						RenderPassConfig passcfg;
						passcfg.colorTargets.push_back({ +windowTexture });
						passcfg.colorTargets[0].clear = false;
						enc->nextPass(passcfg);
						{
							const auto scope = enc->namedScope("gui");
							guiBundle->draw({ windowTexture->resolution(), engineGraphicsDevice(), +enc, +agg });
						}
						agg->submit();
						enc->submit();
					}

					if (scrnshtState == ScreenshotStateEnum::Request)
						takeScreenshot(+windowTexture);
				}

				frameIndex++;
				lastDispatchTime = dispatchTime;

				if (vrFrame)
					vrFrame->commit();
			}
		};
	}

	void EnginePrivateGraphics::emit(uint64 time)
	{
		EnginePrivateGraphicsImpl *impl = (EnginePrivateGraphicsImpl *)this;
		impl->emit(time);
	}

	Holder<Image> EnginePrivateGraphics::screenshot()
	{
		EnginePrivateGraphicsImpl *impl = (EnginePrivateGraphicsImpl *)this;
		return impl->screenshot();
	}

	void EnginePrivateGraphics::initialize()
	{
		EnginePrivateGraphicsImpl *impl = (EnginePrivateGraphicsImpl *)this;
		impl->initialize();
	}

	void EnginePrivateGraphics::finalize()
	{
		EnginePrivateGraphicsImpl *impl = (EnginePrivateGraphicsImpl *)this;
		impl->finalize();
	}

	void EnginePrivateGraphics::dispatch(uint64 time, Holder<GuiRender> guiBundle)
	{
		EnginePrivateGraphicsImpl *impl = (EnginePrivateGraphicsImpl *)this;
		impl->dispatch(time, std::move(guiBundle));
	}

	Holder<EnginePrivateGraphics> newEnginePrivateGraphics(const EngineCreateConfig &config)
	{
		return systemMemory().createImpl<EnginePrivateGraphics, EnginePrivateGraphicsImpl>(config);
	}
}
