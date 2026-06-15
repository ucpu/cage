#include <array>
#include <cstring> // std::strlen
#include <memory>

#include <dawn/native/DawnNative.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "../window/private.h"

#include <cage-core/concurrent.h>
#include <cage-core/debug.h>
#include <cage-core/lineReader.h>
#include <cage-core/profiling.h>
#include <cage-core/timer.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>

namespace cage
{
	namespace
	{
		struct GraphicsContextData : private Immovable
		{
			wgpu::Surface surface;
			Vec2i resolution;
			bool presentable = false;
		};
	}

	namespace privat
	{
		CAGE_API_IMPORT void initializeAbslLogSink();

		struct DeviceBindingsCache;
		struct DevicePipelinesCache;
		struct DeviceBuffersCache;
		struct DeviceTexturesCache;

		Holder<DeviceBindingsCache> newDeviceBindingsCache(GraphicsDevice *device);
		Holder<DevicePipelinesCache> newDevicePipelinesCache(GraphicsDevice *device);
		Holder<DeviceBuffersCache> newDeviceBuffersCache(GraphicsDevice *device);
		Holder<DeviceTexturesCache> newDeviceTexturesCache(GraphicsDevice *device);

		void deviceCacheNextFrame(DeviceBindingsCache *);
		uint32 deviceCacheNextFrame(DevicePipelinesCache *);
		void deviceCacheNextFrame(DeviceBuffersCache *);
		void deviceCacheNextFrame(DeviceTexturesCache *);

		void logImpl(SeverityEnum severity, wgpu::StringView message)
		{
			uint32 len = message.length;
			if (message.length == wgpu::kStrlen)
				len = std::strlen(message.data);
			Holder<LineReader> lr = newLineReader(PointerRange(message.data, message.data + len));
			String l;
			while (lr->readLine(l))
			{
				CAGE_LOG(severity, "wgpu", l);
			}
		}

		struct GraphicsContext : private Immovable
		{
			// the window owns its surface, but the device can destroy all the surfaces in its destructor
			std::shared_ptr<GraphicsContextData> data;
		};
	}

	GraphicsCommandBufferStatistics operator+(const GraphicsCommandBufferStatistics &a, const GraphicsCommandBufferStatistics &b)
	{
		GraphicsCommandBufferStatistics r = a;
		r.passes += b.passes;
		r.pipelineSwitches += b.pipelineSwitches;
		r.drawCalls += b.drawCalls;
		r.primitives += b.primitives;
		return r;
	}

	namespace
	{
		class GraphicsDeviceImpl;

		class GpuFrameTimer : private Immovable
		{
		public:
			GpuFrameTimer(GraphicsDeviceImpl *device);

			void frameStart();
			void frameEnd();

			uint64 time = 0; // microseconds

		private:
			static constexpr uint32 Frames = 8;
			GraphicsDeviceImpl *device = nullptr;
			wgpu::QuerySet querySet = {};
			wgpu::Buffer buffResolve = {};
			std::array<wgpu::Buffer, Frames> buffRead = {};
			uint32 frameIndex = 0;
		};
	}

	namespace
	{
		String conv(wgpu::StringView v)
		{
			if (!v.data)
				return {};
			uint32 len = v.length;
			if (v.length == wgpu::kStrlen)
				len = std::strlen(v.data);
			if (len > 800)
				len = 800;
			return String({ v.data, v.data + len });
		}

		SeverityEnum conv(wgpu::LoggingType type)
		{
			switch (type)
			{
				case wgpu::LoggingType::Verbose:
					return SeverityEnum::Note;
				case wgpu::LoggingType::Info:
					return SeverityEnum::Info;
				case wgpu::LoggingType::Warning:
					return SeverityEnum::Warning;
				case wgpu::LoggingType::Error:
					return SeverityEnum::Error;
				default:
					return SeverityEnum::Critical;
			}
		}

		void logFromInstance(wgpu::LoggingType type, wgpu::StringView message)
		{
			privat::logImpl(conv(type), message);
		}

		void lostFromDevice(const wgpu::Device &device, wgpu::DeviceLostReason reason, wgpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Warning, "graphics", "wgpu device lost:");
			privat::logImpl(SeverityEnum::Note, message);
		}

		void errorFromDevice(const wgpu::Device &device, wgpu::ErrorType error, wgpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Error, "graphics", "wgpu device error:");
			privat::logImpl(SeverityEnum::Error, message);
			detail::debugBreakpoint();
		}

		void logFromDevice(wgpu::LoggingType type, wgpu::StringView message)
		{
			privat::logImpl(conv(type), message);
		}

		class GraphicsDeviceImpl : public GraphicsDevice
		{
		public:
			Holder<Mutex> mutex = newMutex(); // used for the device and queue
			GraphicsDeviceCreateConfig config;
			wgpu::Instance instance;
			wgpu::Adapter adapter;
			wgpu::Device device;
			wgpu::Queue queue;
			Holder<privat::DeviceBindingsCache> bindingsCache;
			Holder<privat::DevicePipelinesCache> pipelinesCache;
			Holder<privat::DeviceBuffersCache> buffersCache;
			Holder<privat::DeviceTexturesCache> texturesCache;
			std::vector<wgpu::CommandBuffer> commands;
			Holder<GpuFrameTimer> gpuTimer;
			Holder<Timer> cpuTimer;
			GraphicsFrameStatistics statistics;
			std::vector<std::weak_ptr<GraphicsContextData>> surfacesCollection;

			void createInstance()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu instance");
				dawn::native::DawnInstanceDescriptor ex = {};
				ex.SetLoggingCallback(logFromInstance);
				wgpu::InstanceDescriptor desc = {};
				desc.nextInChain = &ex;
				static constexpr std::array<wgpu::InstanceFeatureName, 2> requiredFeatures = {
					wgpu::InstanceFeatureName::TimedWaitAny,
					wgpu::InstanceFeatureName::ShaderSourceSPIRV,
				};
				desc.requiredFeatureCount = requiredFeatures.size();
				desc.requiredFeatures = requiredFeatures.data();

				static constexpr std::array<const char *, 2> enabledToggles = {
					"allow_unsafe_apis", //
					"expose_wgsl_experimental_features", //
				};
				static constexpr std::array<const char *, 1> disabledToggles = {
					"timestamp_quantization", //
				};
				wgpu::DawnTogglesDescriptor togglesDesc;
				togglesDesc.enabledToggles = enabledToggles.data();
				togglesDesc.enabledToggleCount = enabledToggles.size();
				togglesDesc.disabledToggles = disabledToggles.data();
				togglesDesc.disabledToggleCount = disabledToggles.size();
				desc.nextInChain = &togglesDesc;

				instance = wgpu::CreateInstance(&desc);
				if (!instance)
					CAGE_THROW_ERROR(Exception, "failed to create wgpu instance");
			}

			void createAdapter()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu adapter");
				wgpu::RequestAdapterOptions opts = {};
				opts.powerPreference = wgpu::PowerPreference::HighPerformance;
				if (config.compatibility)
					opts.compatibleSurface = getContext(config.compatibility)->surface;

				wgpu::Future future = instance.RequestAdapter(&opts, wgpu::CallbackMode::WaitAnyOnly,
					[&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter_, wgpu::StringView message)
					{
						adapter = adapter_;
						CAGE_LOG(SeverityEnum::Info, "wgpu", conv(message));
					});
				instance.WaitAny(future, m);
				if (!adapter)
					CAGE_THROW_ERROR(Exception, "failed to create wgpu adapter");

				wgpu::AdapterInfo info = {};
				adapter.GetInfo(&info);
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter vendor: " + conv(info.vendor));
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter device: " + conv(info.device));
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter description: " + conv(info.description));

				wgpu::Limits limits = {};
				adapter.GetLimits(&limits);
			}

			void createDevice()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu device");
				wgpu::DeviceDescriptor desc = {};
				desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, lostFromDevice);
				desc.SetUncapturedErrorCallback(errorFromDevice);

				static constexpr std::array<wgpu::FeatureName, 2> requiredFeatures = {
					wgpu::FeatureName::TextureCompressionBC,
					wgpu::FeatureName::TimestampQuery,
				};
				desc.requiredFeatures = requiredFeatures.data();
				desc.requiredFeatureCount = requiredFeatures.size();

				wgpu::Limits requiredLimits = {};
				requiredLimits.maxSampledTexturesPerShaderStage = 32;
				requiredLimits.maxSamplersPerShaderStage = 16;
				desc.requiredLimits = &requiredLimits;

				std::vector<const char *> toggles = {
					"vulkan_use_dynamic_rendering", //
					"wait_is_thread_safe", // just to be sure
				};
				//if (!config.vsync)
				//	toggles.push_back("turn_off_vsync");
#ifdef CAGE_DEPLOY
				toggles.push_back("disable_robustness");
				toggles.push_back("skip_validation");
#else
				toggles.push_back("disable_blob_cache");
				toggles.push_back("disable_symbol_renaming"); // preserve variable names in shaders
				toggles.push_back("dump_shaders_on_failure");
				toggles.push_back("enable_immediate_error_handling");
				toggles.push_back("enable_renderdoc_process_injection");
				toggles.push_back("use_user_defined_labels_in_backend"); // show my names for textures etc in nsight
#endif // CAGE_DEPLOY

				wgpu::DawnTogglesDescriptor togglesDesc;
				togglesDesc.enabledToggles = toggles.data();
				togglesDesc.enabledToggleCount = toggles.size();
				desc.nextInChain = &togglesDesc;

				device = adapter.CreateDevice(&desc);
				if (!device)
					CAGE_THROW_ERROR(Exception, "failed to create wgpu device");

				device.SetLoggingCallback(logFromDevice);
			}

			GraphicsDeviceImpl(const GraphicsDeviceCreateConfig &config) : config(config)
			{
				privat::initializeAbslLogSink();
				createInstance();
				createAdapter();
				createDevice();

				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu queue");
				queue = device.GetQueue();
				if (!queue)
					CAGE_THROW_ERROR(Exception, "failed to create wgpu queue");

				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing caches");
				bindingsCache = privat::newDeviceBindingsCache(this);
				pipelinesCache = privat::newDevicePipelinesCache(this);
				buffersCache = privat::newDeviceBuffersCache(this);
				texturesCache = privat::newDeviceTexturesCache(this);
				gpuTimer = systemMemory().createHolder<GpuFrameTimer>(this);
				cpuTimer = newTimer();

				CAGE_LOG(SeverityEnum::Info, "graphics", "wgpu initialized");
			}

			~GraphicsDeviceImpl()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "destroying wgpu device and instance");

				// destroy all window surfaces
				for (auto &it : surfacesCollection)
				{
					if (auto s = it.lock())
					{
						s->surface = {};
						s->resolution = {};
						s->presentable = false;
					}
				}
			}

			void insertCommandBuffer(wgpu::CommandBuffer &&cmds, const GraphicsCommandBufferStatistics &statistics_)
			{
				ScopeLock lock(mutex);
				commands.push_back(std::move(cmds));
				(GraphicsCommandBufferStatistics &)this->statistics = (GraphicsCommandBufferStatistics &)this->statistics + statistics_;
			}

			GraphicsContextData *getContext(Window *window)
			{
				Holder<privat::GraphicsContext> &context = privat::getGraphicsContext(window);
				if (!context)
				{
					ScopeLock lock(mutex);
					CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu surface");
					auto s = std::make_shared<GraphicsContextData>();
					s->surface = wgpu::glfw::CreateSurfaceForWindow(instance, privat::getGlfwWindow(window));
					if (!s->surface)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu surface from window");
					surfacesCollection.push_back(s);
					context = systemMemory().createHolder<privat::GraphicsContext>();
					context->data = s;
				}
				return context->data.get();
			}

			void configureSurface(GraphicsContextData *context, Vec2i resolution)
			{
				ScopeLock lock(mutex);
				context->presentable = false;

				wgpu::SurfaceCapabilities capabilities;
				if (!context->surface.GetCapabilities(adapter, &capabilities))
					CAGE_THROW_ERROR(Exception, "failed to retrieve surface capabilities");

				//bool relaxed = false;
				bool mailbox = false;
				bool immediate = false;
				for (uint32 i = 0; i < capabilities.presentModeCount; i++)
				{
					//if (capabilities.presentModes[i] == wgpu::PresentMode::FifoRelaxed)
					//	relaxed = true;
					if (capabilities.presentModes[i] == wgpu::PresentMode::Mailbox)
						mailbox = true;
					if (capabilities.presentModes[i] == wgpu::PresentMode::Immediate)
						immediate = true;
				}

				wgpu::SurfaceConfiguration cfg = {};
				cfg.device = device;
				cfg.width = resolution[0];
				cfg.height = resolution[1];
				if (config.vsync)
					cfg.presentMode = wgpu::PresentMode::Fifo;
				else
					cfg.presentMode = mailbox ? wgpu::PresentMode::Mailbox : immediate ? wgpu::PresentMode::Immediate : wgpu::PresentMode::Fifo;
				cfg.format = wgpu::TextureFormat::BGRA8Unorm; // this should be guaranteed to work everywhere
				cfg.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
				context->surface.Configure(&cfg);
				context->resolution = resolution;
			}

			Holder<Texture> nextWindow(Window *window)
			{
				const ProfilingScope profiling("next window");

				const Vec2i res = window->resolution();
				if (res[0] <= 0 || res[1] <= 0)
					return {};

				GraphicsContextData *context = getContext(window);
				if (res != context->resolution)
					configureSurface(context, res);

				if (context->presentable)
				{
					const ProfilingScope profiling("present");
					ScopeLock lock(mutex);
					context->surface.Present();
				}

				wgpu::SurfaceTexture tex = {};
				{
					const ProfilingScope profiling("get texture");
					//ScopeLock lock(mutex); // this lock seems to be unnecessary
					context->surface.GetCurrentTexture(&tex);
				}
				switch (tex.status)
				{
					case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
					{
						// todo
						break;
					}
					case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
					case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
					case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
					{
						// todo
						break;
					}
					case wgpu::SurfaceGetCurrentTextureStatus::Lost:
					case wgpu::SurfaceGetCurrentTextureStatus::Error:
					{
						context->presentable = false;
						CAGE_THROW_ERROR(Exception, "surface lost/error");
					}
				}
				CAGE_ASSERT(tex.texture);
				context->presentable = true;

				return newTexture(tex.texture, tex.texture.CreateView(), nullptr, "window surface texture");
			}

			GraphicsFrameStatistics nextFrame()
			{
				const ProfilingScope profiling("next frame");
				{
					const ProfilingScope profiling("process events");
					ScopeLock lock(mutex);
					device.Tick();
					instance.ProcessEvents();
				}
				gpuTimer->frameEnd();
				GraphicsFrameStatistics stats;
				{
					const ProfilingScope profiling("queue submit");
					ScopeLock lock(mutex);
					queue.Submit(commands.size(), commands.data());
					commands.clear();
					std::swap(stats, statistics); // propagate statistics and clear
				}
				gpuTimer->frameStart();
				stats.gpuTime = gpuTimer->time;
				stats.frameTime = cpuTimer->elapsed();
				{
					const ProfilingScope profiling("caches maintenance");
					deviceCacheNextFrame(+bindingsCache);
					stats.pipelinesCompiling = deviceCacheNextFrame(+pipelinesCache);
					deviceCacheNextFrame(+buffersCache);
					deviceCacheNextFrame(+texturesCache);
				}
				return stats;
			}
		};
	}

	namespace
	{
		GpuFrameTimer::GpuFrameTimer(GraphicsDeviceImpl *device) : device(device)
		{
			{
				wgpu::QuerySetDescriptor qsDesc = {};
				qsDesc.type = wgpu::QueryType::Timestamp;
				qsDesc.count = Frames * 2;
				querySet = device->device.CreateQuerySet(&qsDesc);
			}
			{
				wgpu::BufferDescriptor resolveDesc = {};
				resolveDesc.size = Frames * 256; // alignment requirements
				resolveDesc.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;
				buffResolve = device->device.CreateBuffer(&resolveDesc);
			}
			for (uint32 i = 0; i < Frames; i++)
			{
				wgpu::BufferDescriptor readbackDesc = {};
				readbackDesc.size = 2 * sizeof(uint64);
				readbackDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
				buffRead[i] = device->device.CreateBuffer(&readbackDesc);
			}
		}

		void GpuFrameTimer::frameStart()
		{
			const ProfilingScope profiling("gpu timer start");
			ScopeLock lock(device->mutex);
			const uint32 current = frameIndex % Frames;
			auto ce = device->device.CreateCommandEncoder({});
			ce.WriteTimestamp(querySet, current * 2 + 0);
			device->commands.push_back(ce.Finish());
		}

		void GpuFrameTimer::frameEnd()
		{
			const ProfilingScope profiling("gpu timer end");
			ScopeLock lock(device->mutex);
			{
				const uint32 current = frameIndex % Frames;
				if (buffRead[current].GetMapState() != wgpu::BufferMapState::Unmapped)
					return;
				const uint64 offset = current * 256;
				auto ce = device->device.CreateCommandEncoder({});
				ce.WriteTimestamp(querySet, current * 2 + 1);
				ce.ResolveQuerySet(querySet, current * 2, 2, buffResolve, offset);
				ce.CopyBufferToBuffer(buffResolve, offset, buffRead[current], 0, 2 * sizeof(uint64));
				device->commands.push_back(ce.Finish()); // this enques the cmdbuf to be submitted, but does not submit it yet
			}
			if (frameIndex >= Frames)
			{
				const uint32 prev = (frameIndex + Frames - 1) % Frames;
				buffRead[prev].MapAsync(wgpu::MapMode::Read, 0, 2 * sizeof(uint64), wgpu::CallbackMode::AllowProcessEvents,
					[this, prev](wgpu::MapAsyncStatus status, wgpu::StringView)
					{
						if (status != wgpu::MapAsyncStatus::Success)
							return;
						const uint64 *ts = static_cast<const uint64 *>(buffRead[prev].GetConstMappedRange(0, 2 * sizeof(uint64)));
						time = (ts[1] - ts[0]) / 1000; // ns -> us
						buffRead[prev].Unmap();
					});
			}
			frameIndex++;
		}
	}

	namespace privat
	{
		DeviceBindingsCache *getDeviceBindingsCache(GraphicsDevice *device)
		{
			GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)device;
			return +impl->bindingsCache;
		}

		DevicePipelinesCache *getDevicePipelinesCache(GraphicsDevice *device)
		{
			GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)device;
			return +impl->pipelinesCache;
		}

		DeviceBuffersCache *getDeviceBuffersCache(GraphicsDevice *device)
		{
			GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)device;
			return +impl->buffersCache;
		}

		DeviceTexturesCache *getDeviceTexturesCache(GraphicsDevice *device)
		{
			GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)device;
			return +impl->texturesCache;
		}
	}

	Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config)
	{
		return systemMemory().createImpl<GraphicsDevice, GraphicsDeviceImpl>(config);
	}

	void GraphicsDevice::wait(const wgpu::Future &future)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->instance.WaitAny(future, m);
	}

	void GraphicsDevice::insertCommandBuffer(wgpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->insertCommandBuffer(std::move(commands), statistics);
	}

	Holder<Texture> GraphicsDevice::nextWindow(Window *window)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->nextWindow(window);
	}

	GraphicsFrameStatistics GraphicsDevice::nextFrame()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->nextFrame();
	}

	Holder<wgpu::Device> GraphicsDevice::nativeDevice()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		struct LockedDevice : private Noncopyable
		{
			ScopeLock<Mutex> lock;
			wgpu::Device device;
			//ProfilingScope profiling;

			LockedDevice(ScopeLock<Mutex> &&l, wgpu::Device &&d) : lock(std::move(l)), device(std::move(d)) /*, profiling("native device lock") */ {}
		};
		Holder<LockedDevice> l = systemMemory().createHolder<LockedDevice>(ScopeLock(impl->mutex), impl->device);
		return Holder<wgpu::Device>(&l->device, std::move(l));
	}

	Holder<wgpu::Queue> GraphicsDevice::nativeQueue()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		struct LockedQueue : private Noncopyable
		{
			ScopeLock<Mutex> lock;
			wgpu::Queue queue;
			//ProfilingScope profiling;

			LockedQueue(ScopeLock<Mutex> &&l, wgpu::Queue &&q) : lock(std::move(l)), queue(std::move(q)) /*, profiling("native queue lock") */ {}
		};
		Holder<LockedQueue> l = systemMemory().createHolder<LockedQueue>(ScopeLock(impl->mutex), impl->queue);
		return Holder<wgpu::Queue>(&l->queue, std::move(l));
	}
}
