#include <algorithm>
#include <array>
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
#include <cage-engine/texture.h>
#include <cage-engine/window.h>

namespace cage
{
	struct AssetPack;

	namespace
	{
		const ConfigFloat confRenderGamma("cage/graphics/gamma", 2.2);

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

		class EnginePrivateGraphicsImpl : public EnginePrivateGraphics
		{
			Holder<SwapBufferGuard> emitBuffersGuard;
			std::array<EmitBuffer, 3> emitBuffers;
			InterpolationTimingCorrector itc;
			ExclusiveHolder<Texture> sharedTargetTexture;
			Holder<Texture> windowTexture;

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
				return {};

				/*
				Holder<Texture> texture = sharedTargetTexture.get();
				if (!texture)
					return {};

				const Vec2i actualResolution = texture->resolution();
				if (actualResolution[0] <= 0 || actualResolution[1] <= 0)
					return {};

				Vec2i res = actualResolution;
				res[0] = detail::roundUpTo(res[0], 256 / 4);

				Holder<Image> img = newImage();
				img->initialize(res, 4);

				gpu::BufferDescriptor readbackDesc{};
				readbackDesc.usage = gpu::BufferUsage::MapRead | gpu::BufferUsage::CopyDst;
				readbackDesc.size = res[0] * res[1] * 4;
				gpu::Buffer readbackBuffer = engineGraphicsDevice()->nativeDevice()->CreateBuffer(&readbackDesc);

				gpu::TexelCopyTextureInfo srcView = {};
				srcView.texture = texture->nativeTexture();

				gpu::TexelCopyBufferInfo dstBuffer = {};
				dstBuffer.buffer = readbackBuffer;
				dstBuffer.layout.bytesPerRow = res[0] * 4; // must be multiple of 256
				dstBuffer.layout.rowsPerImage = res[1];

				gpu::Extent3D copySize = { (uint32)actualResolution[0], (uint32)actualResolution[1], 1 };

				gpu::CommandEncoder encoder = engineGraphicsDevice()->nativeDevice()->CreateCommandEncoder();
				encoder.CopyTextureToBuffer(&srcView, &dstBuffer, &copySize);
				engineGraphicsDevice()->insertCommandBuffer(encoder.Finish(), {});
				engineGraphicsDevice()->nextFrame();

				gpu::Future future = readbackBuffer.MapAsync(gpu::MapMode::Read, 0, readbackDesc.size, gpu::CallbackMode::WaitAnyOnly,
					[&](gpu::MapAsyncStatus status, gpu::StringView message)
					{
						if (status == gpu::MapAsyncStatus::Success)
						{
							const void *data = readbackBuffer.GetConstMappedRange();
							detail::memcpy((void *)img->rawViewU8().data(), data, readbackDesc.size);
							readbackBuffer.Unmap();
						}
					});
				engineGraphicsDevice()->wait(future);

				// crop padding
				if (res != actualResolution)
				{
					Holder<Image> tmp = newImage();
					tmp->initialize(actualResolution, 4);
					imageBlit(+img, +tmp, {}, {}, actualResolution);
					std::swap(tmp, img);
				}

				// BGR -> RGB
				for (uint32 y = 0; y < actualResolution[1]; y++)
				{
					for (uint32 x = 0; x < actualResolution[0]; x++)
					{
						Vec4 c = img->get4(x, y);
						std::swap(c[0], c[2]);
						img->set(x, y, c);
					}
				}

				return img;
				*/
			}

			// graphics thread ---------------------------------------------------------------------

			void initialize() {}

			void finalize()
			{
				sharedTargetTexture.clear();
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

			Vec2i applyDynamicResolution(Vec2i in) const
			{
				Vec2i res = Vec2i(Vec2(in) * dynamicResolution);
				CAGE_ASSERT(res[0] > 0 && res[1] > 0);
				return res;
			}

			std::vector<SceneRenderCamera> generateCameras(const SceneRenderShared &cfg) const
			{
				std::vector<SceneRenderCamera> cameras;
				if (!cfg.scene)
					return cameras;

				cameras.reserve(cfg.scene->component<CameraComponent>()->count());
				entitiesVisitor(
					[&](Entity *e, const CameraComponent &cam)
					{
						SceneRenderCamera data;
						data.camera = cam;
						data.target = +windowTexture;
						data.resolution = windowTexture->resolution();
						data.cameraSceneMask = e->getOrDefault<SceneComponent>().sceneMask;
						data.effects = e->getOrDefault<ScreenSpaceEffectsComponent>();
						data.effects.gamma = Real(confRenderGamma);
						if (cam.target)
						{
							data.target = cam.target;
							data.resolution = cam.target->resolution();
						}
						if (dynamicResolution != 1)
						{
							data.effects.effects &= ~ScreenSpaceEffectsFlags::AntiAliasing;
							data.resolution = applyDynamicResolution(data.resolution);
						}
						data.transform = modelTransform(e, cfg.interpolationFactor);
						data.projection = initializeProjection(cam, data.resolution);
						data.lodSelection = LodSelection(data.transform.position, cam, data.resolution[1]);
						cameras.push_back(std::move(data));
					},
					cfg.scene, false);

				// ensure render-to-texture before render-to-window
				std::stable_sort(cameras.begin(), cameras.end(), [](const SceneRenderCamera &a, const SceneRenderCamera &b) { return !!a.target < !!b.target; });
				return cameras;
			}

			void dispatch(uint64 dispatchTime, Holder<GuiRender> guiBundle)
			{
				frameStatistics = engineGraphicsDevice()->nextFrame();

				ScopeGuard scopeExit([this]() { windowTexture.clear(); });
				windowTexture = engineGraphicsDevice()->nextWindow(engineWindow());
				updateDynamicResolution();

				if (!windowTexture || !engineAssets()->get<AssetPack>(HashString("cage/cage.pack")))
				{
					sharedTargetTexture.clear();
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
					std::vector<SceneRenderCamera> cameras = generateCameras(cfg.shared);
					cfg.cameras = cameras;
					const auto commands = sceneRender(cfg);
					for (const auto &cmd : commands)
						cmd->submit();
				}

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

				// purposufully make the target texture available only after the whole frame has been submitted
				sharedTargetTexture.assign(windowTexture.share());

				frameIndex++;
				lastDispatchTime = dispatchTime;
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
