#include <array>
#include <cstring> // std::strlen

#include <dawn/native/DawnNative.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "../window/private.h"

#include <cage-core/concurrent.h>
#include <cage-core/debug.h>
#include <cage-core/lineReader.h>
#include <cage-core/profiling.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>

namespace cage
{
	CAGE_API_IMPORT void initializeAbslLogSink();

	GraphicsFrameStatistics operator+(const GraphicsFrameStatistics &a, const GraphicsFrameStatistics &b)
	{
		GraphicsFrameStatistics r = a;
		r.passes += b.passes;
		r.pipelineSwitches += b.pipelineSwitches;
		r.drawCalls += b.drawCalls;
		r.primitives += b.primitives;
		return r;
	}

	namespace privat
	{
		struct DeviceBindingsCache;
		struct DevicePipelinesCache;
		struct DeviceBuffersCache;
		struct DeviceTexturesCache;

		Holder<DeviceBindingsCache> newDeviceBindingsCache(GraphicsDevice *device);
		Holder<DevicePipelinesCache> newDevicePipelinesCache(GraphicsDevice *device);
		Holder<DeviceBuffersCache> newDeviceBuffersCache(GraphicsDevice *device);
		Holder<DeviceTexturesCache> newDeviceTexturesCache(GraphicsDevice *device);

		void deviceCacheNextFrame(DeviceBindingsCache *);
		void deviceCacheNextFrame(DevicePipelinesCache *);
		void deviceCacheNextFrame(DeviceBuffersCache *);
		void deviceCacheNextFrame(DeviceTexturesCache *);

		struct GraphicsContext : private Immovable
		{
			wgpu::Surface surface;
			Vec2i resolution;
			bool presentable = false;
		};
	}

	namespace
	{
		class GraphicsDeviceImpl;

		class GpuFrameTimer : private Immovable
		{
		public:
			GpuFrameTimer(GraphicsDeviceImpl *device, const wgpu::Instance &instance);

			void beginFrame();
			void endFrame();

			// time spent processing the frame
			uint64 execution() const
			{
				const auto &d = data[frameIndex % data.size()];
				return d.frameEnd - d.frameBegin;
			}

			// time elapsed start-to-start
			uint64 duration() const { return (data[frameIndex % data.size()].frameBegin - data[(frameIndex + data.size() - 1) % data.size()].frameBegin); }

		private:
			struct Data
			{
				wgpu::Buffer buffResolve, buffRead;
				uint64 frameBegin = 0; // microsecond timestamp
				uint64 frameEnd = 0; // microsecond timestamp
			};

			GraphicsDeviceImpl *device = nullptr;
			wgpu::Instance instance;
			wgpu::QuerySet querySet;
			std::array<Data, 5> data = {};
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

		void logFromInstance(wgpu::LoggingType type, wgpu::StringView message)
		{
			logImpl(conv(type), message);
		}

		void lostFromDevice(const wgpu::Device &device, wgpu::DeviceLostReason reason, wgpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Warning, "graphics", "wgpu device lost:");
			logImpl(SeverityEnum::Note, message);
		}

		void errorFromDevice(const wgpu::Device &device, wgpu::ErrorType error, wgpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Error, "graphics", "wgpu device error:");
			logImpl(SeverityEnum::Error, message);
			detail::debugBreakpoint();
		}

		void logFromDevice(wgpu::LoggingType type, wgpu::StringView message)
		{
			logImpl(conv(type), message);
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
			Holder<GpuFrameTimer> frameTimer;
			Holder<privat::DeviceBindingsCache> bindingsCache;
			Holder<privat::DevicePipelinesCache> pipelinesCache;
			Holder<privat::DeviceBuffersCache> buffersCache;
			Holder<privat::DeviceTexturesCache> texturesCache;
			std::vector<wgpu::CommandBuffer> commands;
			GraphicsFrameStatistics statistics;

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

				static constexpr std::array<const char *, 2> toggles = {
					"allow_unsafe_apis", //
					"expose_wgsl_experimental_features", //
				};
				wgpu::DawnTogglesDescriptor togglesDesc;
				togglesDesc.enabledToggles = toggles.data();
				togglesDesc.enabledToggleCount = toggles.size();
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
					"disable_symbol_renaming", // preserve variable names in shaders
					"dump_shaders_on_failure", //
					"use_user_defined_labels_in_backend", // show my names for textures etc in nsight
					"wait_is_thread_safe", // just to be sure
				};
				if (!config.vsync)
					toggles.push_back("turn_off_vsync");
#ifdef CAGE_DEPLOY
				toggles.push_back("enable_immediate_error_handling");
				toggles.push_back("skip_validation");
#else
				toggles.push_back("disable_blob_cache");
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
				initializeAbslLogSink();
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
				frameTimer = systemMemory().createHolder<GpuFrameTimer>(this, instance);

				CAGE_LOG(SeverityEnum::Info, "graphics", "wgpu initialized");
			}

			~GraphicsDeviceImpl() { CAGE_LOG(SeverityEnum::Info, "graphics", "destroying wgpu device and instance"); }

			void processEvents()
			{
				const ProfilingScope profiling("graphics process events");
				ScopeLock lock(mutex);
				device.Tick();
			}

			void insertCommandBuffer(wgpu::CommandBuffer &&cmds, const GraphicsFrameStatistics &statistics_)
			{
				ScopeLock lock(mutex);
				commands.push_back(std::move(cmds));
				this->statistics = this->statistics + statistics_;
			}

			GraphicsFrameStatistics submitCommandBuffers()
			{
				GraphicsFrameStatistics stats;
				const ProfilingScope profiling("queue submit");
				ScopeLock lock(mutex);
				queue.Submit(commands.size(), commands.data());
				commands.clear();
				std::swap(stats, statistics); // propagate statistics and clear
				return stats;
			}

			privat::GraphicsContext *getContext(Window *window)
			{
				Holder<privat::GraphicsContext> &context = privat::getGlfwContext(window);
				if (!context)
					context = systemMemory().createHolder<privat::GraphicsContext>();
				if (!context->surface)
				{
					CAGE_LOG(SeverityEnum::Info, "graphics", "initializing wgpu surface");
					context->surface = wgpu::glfw::CreateSurfaceForWindow(instance, getGlfwWindow(window));
					if (!context->surface)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu surface from window");
				}
				return +context;
			}

			void configureSurface(privat::GraphicsContext *context, Vec2i resolution)
			{
				ScopeLock lock(mutex);
				context->presentable = false;

				wgpu::SurfaceCapabilities capabilities;
				if (!context->surface.GetCapabilities(adapter, &capabilities))
					CAGE_THROW_ERROR(Exception, "failed to retrieve surface capabilities");

				bool relaxed = false;
				for (uint32 i = 0; i < capabilities.presentModeCount; i++)
					if (capabilities.presentModes[i] == wgpu::PresentMode::FifoRelaxed)
						relaxed = true;

				wgpu::SurfaceConfiguration cfg = {};
				cfg.device = device;
				cfg.width = resolution[0];
				cfg.height = resolution[1];
				cfg.presentMode = relaxed ? wgpu::PresentMode::FifoRelaxed : wgpu::PresentMode::Fifo;
				cfg.format = wgpu::TextureFormat::BGRA8Unorm; // this should be guaranteed to work everywhere
				cfg.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
				context->surface.Configure(&cfg);
				context->resolution = resolution;
			}

			GraphicsFrameData nextFrame(Window *window)
			{
				const ProfilingScope profiling("next frame");

				frameTimer->endFrame();

				processEvents();

				const GraphicsFrameStatistics stats = submitCommandBuffers();

				{
					const ProfilingScope profiling("caches maintenance");
					deviceCacheNextFrame(+bindingsCache);
					deviceCacheNextFrame(+pipelinesCache);
					deviceCacheNextFrame(+buffersCache);
					deviceCacheNextFrame(+texturesCache);
				}

				const Vec2i res = window->resolution();
				CAGE_ASSERT(res[0] >= 0 && res[1] >= 0);
				if (res[0] * res[1] == 0)
					return {};

				privat::GraphicsContext *context = getContext(window);
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
					case wgpu::SurfaceGetCurrentTextureStatus::Error:
					case wgpu::SurfaceGetCurrentTextureStatus::Lost:
						context->presentable = false;
						CAGE_THROW_ERROR(Exception, "surface lost/error");
					default:
						// todo handle other options
						break;
				}
				CAGE_ASSERT(tex.texture);
				context->presentable = true;

				GraphicsFrameData fd;
				(GraphicsFrameStatistics &)fd = stats;
				fd.targetTexture = newTexture(tex.texture, tex.texture.CreateView(), nullptr, "surface texture");
				fd.frameExecution = frameTimer->execution();
				fd.frameDuration = frameTimer->duration();
				frameTimer->beginFrame();
				return fd;
			}
		};
	}

	namespace
	{
		GpuFrameTimer::GpuFrameTimer(GraphicsDeviceImpl *device, const wgpu::Instance &instance) : device(device), instance(instance)
		{
			wgpu::QuerySetDescriptor qsDesc = {};
			qsDesc.type = wgpu::QueryType::Timestamp;
			qsDesc.count = 2; // begin frame, end frame
			querySet = device->device.CreateQuerySet(&qsDesc);

			wgpu::BufferDescriptor resolveDesc{};
			resolveDesc.size = 2 * sizeof(uint64);
			resolveDesc.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;

			wgpu::BufferDescriptor readbackDesc{};
			readbackDesc.size = 2 * sizeof(uint64);
			readbackDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;

			for (auto &it : data)
			{
				it.buffResolve = device->device.CreateBuffer(&resolveDesc);
				it.buffRead = device->device.CreateBuffer(&readbackDesc);
			}
		}

		void GpuFrameTimer::beginFrame()
		{
			const ProfilingScope profiling("begin frame");

			ScopeLock lock(device->mutex);

			auto ce = device->device.CreateCommandEncoder({});
			ce.WriteTimestamp(querySet, 0);
			device->commands.push_back(ce.Finish());
		}

		void GpuFrameTimer::endFrame()
		{
			const ProfilingScope profiling("end frame");

			ScopeLock lock(device->mutex);

			const uint32 current = frameIndex % data.size();
			const uint32 oldest = (frameIndex + 1) % data.size();

			{
				auto ce = device->device.CreateCommandEncoder({});
				ce.WriteTimestamp(querySet, 1);
				ce.ResolveQuerySet(querySet, 0, 2, data[current].buffResolve, 0);
				ce.CopyBufferToBuffer(data[current].buffResolve, 0, data[current].buffRead, 0, 2 * sizeof(uint64));
				device->commands.push_back(ce.Finish());
			}

			if (frameIndex > data.size())
			{
				wgpu::Buffer &buffer = data[oldest].buffRead;
				wgpu::Future future = buffer.MapAsync(wgpu::MapMode::Read, 0, 2 * sizeof(uint64), wgpu::CallbackMode::WaitAnyOnly,
					[&](wgpu::MapAsyncStatus status, wgpu::StringView message)
					{
						if (status != wgpu::MapAsyncStatus::Success)
							return;

						const uint64 *timestamps = static_cast<const uint64 *>(buffer.GetConstMappedRange(0, 2 * sizeof(uint64)));

						data[oldest].frameBegin = timestamps[0] / 1'000; // convert ns to us
						data[oldest].frameEnd = timestamps[1] / 1'000; // convert ns to us

						buffer.Unmap();
					});
				instance.WaitAny(future, m);
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

	void GraphicsDevice::processEvents()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->processEvents();
	}

	void GraphicsDevice::insertCommandBuffer(wgpu::CommandBuffer &&commands, const GraphicsFrameStatistics &statistics)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->insertCommandBuffer(std::move(commands), statistics);
	}

	void GraphicsDevice::submitCommandBuffers()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->submitCommandBuffers();
	}

	GraphicsFrameData GraphicsDevice::nextFrame(Window *window)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->nextFrame(window);
	}

	void GraphicsDevice::wait(const wgpu::Future &future)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->instance.WaitAny(future, m);
	}

	Holder<wgpu::Device> GraphicsDevice::nativeDeviceNoLock()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return Holder<wgpu::Device>(&impl->device, nullptr);
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
