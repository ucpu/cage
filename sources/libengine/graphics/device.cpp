#include <array>
#include <cstring> // std::strlen
#include <memory>

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

/*
namespace cage
{
	namespace
	{
		struct GraphicsContextData : private Immovable
		{
			gpu::Surface surface;
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

		void logImpl(SeverityEnum severity, gpu::StringView message)
		{
			uint32 len = message.length;
			if (message.length == gpu::kStrlen)
				len = std::strlen(message.data);
			Holder<LineReader> lr = newLineReader(PointerRange(message.data, message.data + len));
			String l;
			while (lr->readLine(l))
			{
				CAGE_LOG(severity, "wgpu", l);
			}
		}

		struct WindowGpuContext : private Immovable
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
			gpu::QuerySet querySet = {};
			gpu::Buffer buffResolve = {};
			std::array<gpu::Buffer, Frames> buffRead = {};
			uint32 frameIndex = 0;
		};
	}

	namespace
	{
		String conv(gpu::StringView v)
		{
			if (!v.data)
				return {};
			uint32 len = v.length;
			if (v.length == gpu::kStrlen)
				len = std::strlen(v.data);
			if (len > 800)
				len = 800;
			return String({ v.data, v.data + len });
		}

		SeverityEnum conv(gpu::LoggingType type)
		{
			switch (type)
			{
				case gpu::LoggingType::Verbose:
					return SeverityEnum::Note;
				case gpu::LoggingType::Info:
					return SeverityEnum::Info;
				case gpu::LoggingType::Warning:
					return SeverityEnum::Warning;
				case gpu::LoggingType::Error:
					return SeverityEnum::Error;
				default:
					return SeverityEnum::Critical;
			}
		}

		void logFromInstance(gpu::LoggingType type, gpu::StringView message)
		{
			privat::logImpl(conv(type), message);
		}

		void lostFromDevice(const gpu::Device &device, gpu::DeviceLostReason reason, gpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Warning, "graphics", "wgpu device lost:");
			privat::logImpl(SeverityEnum::Note, message);
		}

		void errorFromDevice(const gpu::Device &device, gpu::ErrorType error, gpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Error, "graphics", "wgpu device error:");
			privat::logImpl(SeverityEnum::Error, message);
			detail::debugBreakpoint();
		}

		void logFromDevice(gpu::LoggingType type, gpu::StringView message)
		{
			privat::logImpl(conv(type), message);
		}

		class GraphicsDeviceImpl : public GraphicsDevice
		{
		public:
			Holder<Mutex> mutex = newMutex(); // used for the device and queue
			GraphicsDeviceCreateConfig config;
			gpu::Instance instance;
			gpu::Adapter adapter;
			gpu::Device device;
			gpu::Queue queue;
			Holder<privat::DeviceBindingsCache> bindingsCache;
			Holder<privat::DevicePipelinesCache> pipelinesCache;
			Holder<privat::DeviceBuffersCache> buffersCache;
			Holder<privat::DeviceTexturesCache> texturesCache;
			std::vector<gpu::CommandBuffer> commands;
			Holder<GpuFrameTimer> gpuTimer;
			Holder<Timer> cpuTimer;
			GraphicsFrameStatistics statistics;
			std::vector<std::weak_ptr<GraphicsContextData>> surfacesCollection;

			void createInstance()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu instance");
				dawn::native::DawnInstanceDescriptor ex = {};
				ex.SetLoggingCallback(logFromInstance);
				gpu::InstanceDescriptor desc = {};
				desc.nextInChain = &ex;
				static constexpr std::array<gpu::InstanceFeatureName, 2> requiredFeatures = {
					gpu::InstanceFeatureName::TimedWaitAny,
					gpu::InstanceFeatureName::ShaderSourceSPIRV,
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
				gpu::DawnTogglesDescriptor togglesDesc;
				togglesDesc.enabledToggles = enabledToggles.data();
				togglesDesc.enabledToggleCount = enabledToggles.size();
				togglesDesc.disabledToggles = disabledToggles.data();
				togglesDesc.disabledToggleCount = disabledToggles.size();
				desc.nextInChain = &togglesDesc;

				instance = gpu::CreateInstance(&desc);
				if (!instance)
					CAGE_THROW_ERROR(Exception, "failed to create wgpu instance");
			}

			void createAdapter()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu adapter");
				gpu::RequestAdapterOptions opts = {};
				opts.powerPreference = gpu::PowerPreference::HighPerformance;
				if (config.compatibility)
					opts.compatibleSurface = getContext(config.compatibility)->surface;

				gpu::Future future = instance.RequestAdapter(&opts, gpu::CallbackModeEnum::WaitAnyOnly,
					[&](gpu::RequestAdapterStatus status, gpu::Adapter adapter_, gpu::StringView message)
					{
						adapter = adapter_;
						CAGE_LOG(SeverityEnum::Info, "wgpu", conv(message));
					});
				instance.WaitAny(future, m);
				if (!adapter)
					CAGE_THROW_ERROR(Exception, "failed to create wgpu adapter");

				gpu::AdapterInfo info = {};
				adapter.GetInfo(&info);
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter vendor: " + conv(info.vendor));
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter device: " + conv(info.device));
				CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter description: " + conv(info.description));

				gpu::Limits limits = {};
				adapter.GetLimits(&limits);
			}

			void createDevice()
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu device");
				gpu::DeviceDescriptor desc = {};
				desc.SetDeviceLostCallback(gpu::CallbackModeEnum::AllowSpontaneous, lostFromDevice);
				desc.SetUncapturedErrorCallback(errorFromDevice);

				static constexpr std::array<gpu::FeatureName, 2> requiredFeatures = {
					gpu::FeatureName::TextureCompressionBC,
					gpu::FeatureName::TimestampQuery,
				};
				desc.requiredFeatures = requiredFeatures.data();
				desc.requiredFeatureCount = requiredFeatures.size();

				gpu::Limits requiredLimits = {};
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

				gpu::DawnTogglesDescriptor togglesDesc;
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

			void insertCommandBuffer(gpu::CommandBuffer &&cmds, const GraphicsCommandBufferStatistics &statistics_)
			{
				ScopeLock lock(mutex);
				commands.push_back(std::move(cmds));
				(GraphicsCommandBufferStatistics &)this->statistics = (GraphicsCommandBufferStatistics &)this->statistics + statistics_;
			}

			GraphicsContextData *getContext(Window *window)
			{
				Holder<privat::WindowGpuContext> &context = privat::getWindowGpuContext(window);
				if (!context)
				{
					ScopeLock lock(mutex);
					CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu surface");
					auto s = std::make_shared<GraphicsContextData>();
					s->surface = gpu::glfw::CreateSurfaceForWindow(instance, privat::getGlfwWindow(window));
					if (!s->surface)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu surface from window");
					surfacesCollection.push_back(s);
					context = systemMemory().createHolder<privat::WindowGpuContext>();
					context->data = s;
				}
				return context->data.get();
			}

			void configureSurface(GraphicsContextData *context, Vec2i resolution)
			{
				ScopeLock lock(mutex);
				context->presentable = false;

				gpu::SurfaceCapabilities capabilities;
				if (!context->surface.GetCapabilities(adapter, &capabilities))
					CAGE_THROW_ERROR(Exception, "failed to retrieve surface capabilities");

				//bool relaxed = false;
				bool mailbox = false;
				bool immediate = false;
				for (uint32 i = 0; i < capabilities.presentModeCount; i++)
				{
					//if (capabilities.presentModes[i] == gpu::PresentMode::FifoRelaxed)
					//	relaxed = true;
					if (capabilities.presentModes[i] == gpu::PresentMode::Mailbox)
						mailbox = true;
					if (capabilities.presentModes[i] == gpu::PresentMode::Immediate)
						immediate = true;
				}

				gpu::SurfaceConfiguration cfg = {};
				cfg.device = device;
				cfg.width = resolution[0];
				cfg.height = resolution[1];
				if (config.vsync)
					cfg.presentMode = gpu::PresentMode::Fifo;
				else
					cfg.presentMode = mailbox ? gpu::PresentMode::Mailbox : immediate ? gpu::PresentMode::Immediate : gpu::PresentMode::Fifo;
				cfg.format = gpu::TextureFormatEnum::BGRA8Unorm; // this should be guaranteed to work everywhere
				cfg.usage = gpu::TextureUsageFlags::RenderAttachment | gpu::TextureUsageFlags::CopySrc;
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

				gpu::SurfaceTexture tex = {};
				{
					const ProfilingScope profiling("get texture");
					//ScopeLock lock(mutex); // this lock seems to be unnecessary
					context->surface.GetCurrentTexture(&tex);
				}
				switch (tex.status)
				{
					case gpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
					{
						// todo
						break;
					}
					case gpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
					case gpu::SurfaceGetCurrentTextureStatus::Timeout:
					case gpu::SurfaceGetCurrentTextureStatus::Outdated:
					{
						// todo
						break;
					}
					case gpu::SurfaceGetCurrentTextureStatus::Lost:
					case gpu::SurfaceGetCurrentTextureStatus::Error:
					{
						context->presentable = false;
						CAGE_THROW_ERROR(Exception, "surface lost/error");
					}
				}
				CAGE_ASSERT(tex.texture);
				context->presentable = true;

				return newTexture(tex.texture, tex.texture.createView(), nullptr, "window surface texture");
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
				gpu::QuerySetDescriptor qsDesc = {};
				qsDesc.type = gpu::QueryType::Timestamp;
				qsDesc.count = Frames * 2;
				querySet = device->device.CreateQuerySet(&qsDesc);
			}
			{
				gpu::BufferDescriptor resolveDesc = {};
				resolveDesc.size = Frames * 256; // alignment requirements
				resolveDesc.usage = gpu::BufferUsageFlags::QueryResolve | gpu::BufferUsageFlags::CopySrc;
				buffResolve = device->device.createBuffer(&resolveDesc);
			}
			for (uint32 i = 0; i < Frames; i++)
			{
				gpu::BufferDescriptor readbackDesc = {};
				readbackDesc.size = 2 * sizeof(uint64);
				readbackDesc.usage = gpu::BufferUsageFlags::MapRead | gpu::BufferUsageFlags::CopyDst;
				buffRead[i] = device->device.createBuffer(&readbackDesc);
			}
		}

		void GpuFrameTimer::frameStart()
		{
#ifndef CAGE_SYSTEM_MAC
			const ProfilingScope profiling("gpu timer start");
			ScopeLock lock(device->mutex);
			const uint32 current = frameIndex % Frames;
			auto ce = device->device.createCommandEncoder({});
			ce.WriteTimestamp(querySet, current * 2 + 0);
			device->commands.push_back(ce.finish());
#endif // CAGE_SYSTEM_MAC
		}

		void GpuFrameTimer::frameEnd()
		{
#ifndef CAGE_SYSTEM_MAC
			const ProfilingScope profiling("gpu timer end");
			ScopeLock lock(device->mutex);
			{
				const uint32 current = frameIndex % Frames;
				if (buffRead[current].GetMapState() != gpu::BufferMapState::Unmapped)
					return;
				const uint64 offset = current * 256;
				auto ce = device->device.createCommandEncoder({});
				ce.WriteTimestamp(querySet, current * 2 + 1);
				ce.ResolveQuerySet(querySet, current * 2, 2, buffResolve, offset);
				ce.CopyBufferToBuffer(buffResolve, offset, buffRead[current], 0, 2 * sizeof(uint64));
				device->commands.push_back(ce.finish()); // this enques the cmdbuf to be submitted, but does not submit it yet
			}
			if (frameIndex >= Frames)
			{
				const uint32 prev = (frameIndex + Frames - 1) % Frames;
				buffRead[prev].mapAsync(gpu::MapModeEnum::Read, 0, 2 * sizeof(uint64), gpu::CallbackModeEnum::AllowProcessEvents,
					[this, prev](gpu::MapAsyncStatus status, gpu::StringView)
					{
						if (status != gpu::MapAsyncStatus::Success)
							return;
						const uint64 *ts = static_cast<const uint64 *>(buffRead[prev].GetConstMappedRange(0, 2 * sizeof(uint64)));
						time = (ts[1] - ts[0]) / 1000; // ns -> us
						buffRead[prev].unmap();
						ProfilingEvent ev = profilingEventBegin("gpu", ProfilingFrameTag());
						profilingEventEnd(ev, time);
					});
			}
#endif // CAGE_SYSTEM_MAC
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

	void GraphicsDevice::wait(const gpu::Future &future)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->instance.WaitAny(future, m);
	}

	void GraphicsDevice::insertCommandBuffer(gpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics)
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

	Holder<gpu::Device> GraphicsDevice::nativeDevice()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		struct LockedDevice : private Noncopyable
		{
			ScopeLock<Mutex> lock;
			gpu::Device device;
			//ProfilingScope profiling;

			LockedDevice(ScopeLock<Mutex> &&l, gpu::Device d) : lock(std::move(l)), device(std::move(d)) //, profiling("native device lock")
			{}
		};
		Holder<LockedDevice> l = systemMemory().createHolder<LockedDevice>(ScopeLock(impl->mutex), impl->device);
		return Holder<gpu::Device>(&l->device, std::move(l));
	}

	Holder<gpu::Queue> GraphicsDevice::nativeQueue()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		struct LockedQueue : private Noncopyable
		{
			ScopeLock<Mutex> lock;
			gpu::Queue queue;
			//ProfilingScope profiling;

			LockedQueue(ScopeLock<Mutex> &&l, gpu::Queue q) : lock(std::move(l)), queue(std::move(q)) //, profiling("native queue lock")
			{}
		};
		Holder<LockedQueue> l = systemMemory().createHolder<LockedQueue>(ScopeLock(impl->mutex), impl->queue);
		return Holder<gpu::Queue>(&l->queue, std::move(l));
	}
}
*/

namespace cage
{
	namespace
	{
		struct DummyDevice : public GraphicsDevice
		{
			gpu::Device dev;

			DummyDevice(const GraphicsDeviceCreateConfig &config)
			{
				gpu::GpuDeviceDescriptor desc;
				desc.window = config.compatibility;
				dev = gpu::newGpuDevice(desc);
			}
		};
	}

	namespace privat
	{
		struct DeviceBindingsCache;
		struct DevicePipelinesCache;
		struct DeviceBuffersCache;
		struct DeviceTexturesCache;

		DeviceBindingsCache *getDeviceBindingsCache(GraphicsDevice *device)
		{
			return {}; // todo
		}

		DevicePipelinesCache *getDevicePipelinesCache(GraphicsDevice *device)
		{
			return {}; // todo
		}

		DeviceBuffersCache *getDeviceBuffersCache(GraphicsDevice *device)
		{
			return {}; // todo
		}

		DeviceTexturesCache *getDeviceTexturesCache(GraphicsDevice *device)
		{
			return {}; // todo
		}
	}

	void GraphicsDevice::wait(const gpu::Future &future)
	{
		// todo
	}

	void GraphicsDevice::insertCommandBuffer(gpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics)
	{
		// todo
	}

	Holder<Texture> GraphicsDevice::nextWindow(Window *window)
	{
		return {}; // todo
	}

	GraphicsFrameStatistics GraphicsDevice::nextFrame()
	{
		return {}; // todo
	}

	Holder<gpu::Device> GraphicsDevice::nativeDevice()
	{
		DummyDevice *impl = (DummyDevice *)this;
		return Holder<gpu::Device>(&impl->dev, nullptr);
	}

	Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config)
	{
		return systemMemory().createImpl<GraphicsDevice, DummyDevice>(config);
	}
}
