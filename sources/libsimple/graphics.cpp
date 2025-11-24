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
			Holder<EntityManager> scene = newEntityManager({ .linearAllocators = true });
			uint64 emitTime = 0;
		};

		struct CameraData : SceneRenderConfig
		{
			Holder<PointerRange<Holder<GraphicsEncoder>>> encoders;
			bool finalProduct = false; // ensure render-to-texture before render-to-window

			void operator()() { encoders = sceneRender(*this); }

			bool operator<(const CameraData &other) const { return finalProduct < other.finalProduct; }
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

			uint64 lastDispatchTime = 0;
			uint32 frameIndex = 0;
			//uint32 nextAllowedDrFrameIndex = 100; // do not update DR at the very start

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
					EntitiesCopyConfig cfg;
					cfg.source = engineEntities();
					cfg.destination = +emitBuffers[lock.index()].scene;
					entitiesCopy(cfg);
					emitBuffers[lock.index()].emitTime = emitTime;
				}
			}

			Holder<Image> screenshot()
			{
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

				wgpu::BufferDescriptor readbackDesc{};
				readbackDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
				readbackDesc.size = res[0] * res[1] * 4;
				wgpu::Buffer readbackBuffer = engineGraphicsDevice()->nativeDevice()->CreateBuffer(&readbackDesc);

				wgpu::TexelCopyTextureInfo srcView = {};
				srcView.texture = texture->nativeTexture();

				wgpu::TexelCopyBufferInfo dstBuffer = {};
				dstBuffer.buffer = readbackBuffer;
				dstBuffer.layout.bytesPerRow = res[0] * 4; // must be multiple of 256
				dstBuffer.layout.rowsPerImage = res[1];

				wgpu::Extent3D copySize = { (uint32)actualResolution[0], (uint32)actualResolution[1], 1 };

				wgpu::CommandEncoder encoder = engineGraphicsDevice()->nativeDevice()->CreateCommandEncoder();
				encoder.CopyTextureToBuffer(&srcView, &dstBuffer, &copySize);
				engineGraphicsDevice()->insertCommandBuffer(encoder.Finish(), {});
				engineGraphicsDevice()->submitCommandBuffers();

				wgpu::Future future = readbackBuffer.MapAsync(wgpu::MapMode::Read, 0, readbackDesc.size, wgpu::CallbackMode::WaitAnyOnly,
					[&](wgpu::MapAsyncStatus status, wgpu::StringView message)
					{
						if (status == wgpu::MapAsyncStatus::Success)
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
			}

			// graphics thread ---------------------------------------------------------------------

			void initialize() {}

			void finalize() { sharedTargetTexture.clear(); }

			void renderCameras(const SceneRenderConfig &cfg)
			{
				std::vector<CameraData> cameras;

				entitiesVisitor(
					[&](Entity *e, const CameraComponent &cam)
					{
						CameraData data;
						(SceneRenderConfig &)data = cfg;
						data.camera = cam;
						data.cameraSceneMask = e->getOrDefault<SceneComponent>().sceneMask;
						data.effects = e->getOrDefault<ScreenSpaceEffectsComponent>();
						//if (dynamicResolution != 1)
						//	data.effects.effects &= ~ScreenSpaceEffectsFlags::AntiAliasing;
						data.effects.gamma = Real(confRenderGamma);
						if (cam.target)
						{
							data.target = cam.target;
							data.resolution = cam.target->resolution();
						}
						else
						{
							//data.resolution = applyDynamicResolution(cfg.resolution);
						}
						data.transform = modelTransform(e, cfg.interpolationFactor);
						data.projection = initializeProjection(cam, data.resolution);
						data.lodSelection = LodSelection(data.transform.position, cam, data.resolution[1]);
						data.finalProduct = !cam.target;
						cameras.push_back(std::move(data));
					},
					cfg.scene, false);

				std::stable_sort(cameras.begin(), cameras.end()); // ensure render-to-texture before render-to-window
				tasksRunBlocking<CameraData>("render scene", cameras);

				for (const auto &cam : cameras)
					for (const auto &it : cam.encoders)
						it->submit();
			}

			void dispatch(uint64 dispatchTime, Holder<GuiRender> guiBundle)
			{
				const GraphicsFrameData frameData = engineGraphicsDevice()->nextFrame(engineWindow());
				gpuTime = frameData.frameExecution;
				drawCalls = frameData.drawCalls;
				drawPrimitives = frameData.primitives;
				dynamicResolution = 1;

				if (!frameData.targetTexture || !engineAssets()->get<AssetPack>(HashString("cage/cage.pack")))
				{
					sharedTargetTexture.clear();
					return;
				}

				if (auto lock = emitBuffersGuard->read())
				{
					//updateDynamicResolution();
					const EmitBuffer &eb = emitBuffers[lock.index()];
					SceneRenderConfig cfg;
					cfg.device = engineGraphicsDevice();
					cfg.assets = engineAssets();
					cfg.onDemand = engineAssetsOnDemand();
					cfg.scene = +eb.scene;
					cfg.target = +frameData.targetTexture;
					cfg.resolution = frameData.targetTexture->resolution();
					if (!graphicsThread().disableTimePassage)
					{
						const uint64 period = controlThread().updatePeriod();
						cfg.currentTime = itc(eb.emitTime, dispatchTime, period);
						cfg.elapsedTime = dispatchTime - lastDispatchTime;
						cfg.interpolationFactor = saturate(Real(cfg.currentTime - eb.emitTime) / period);
						cfg.frameIndex = frameIndex;
					}
					renderCameras(cfg);
					frameIndex++;
					lastDispatchTime = dispatchTime;
				}

				if (guiBundle)
				{
					Holder<GraphicsEncoder> enc = newGraphicsEncoder(engineGraphicsDevice(), "gui");
					Holder<GraphicsAggregateBuffer> agg = newGraphicsAggregateBuffer({ engineGraphicsDevice() });
					RenderPassConfig passcfg;
					passcfg.colorTargets.push_back({ +frameData.targetTexture });
					passcfg.colorTargets[0].clear = false;
					enc->nextPass(passcfg);
					{
						const auto scope = enc->namedScope("gui");
						guiBundle->draw({ frameData.targetTexture->resolution(), engineGraphicsDevice(), +enc, +agg });
					}
					agg->submit();
					enc->submit();
				}

				// purposufully make the target texture available only after the whole frame has been submitted
				sharedTargetTexture.assign(frameData.targetTexture.share());
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
