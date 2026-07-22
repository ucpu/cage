#include <svector.h>
#include <vector>

#include <cage-core/concurrent.h>
#include <cage-core/profiling.h>
#include <cage-core/timer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>

namespace cage
{
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
		/*
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
			gpu::QuerySet querySet;
			gpu::Buffer buffResolve;
			std::array<gpu::Buffer, Frames> buffRead = {};
			uint32 frameIndex = 0;
		};
		*/
	}

	namespace
	{
		class GraphicsDeviceImpl : public GraphicsDevice
		{
		public:
			Holder<Mutex> mutex = newMutex(); // used for the device and queue
			GraphicsDeviceCreateConfig config;
			gpu::Device device;
			Holder<privat::DeviceBindingsCache> bindingsCache;
			Holder<privat::DevicePipelinesCache> pipelinesCache;
			Holder<privat::DeviceBuffersCache> buffersCache;
			Holder<privat::DeviceTexturesCache> texturesCache;
			std::vector<gpu::CommandBuffer> commands;
			//Holder<GpuFrameTimer> gpuTimer;
			Holder<Timer> cpuTimer;
			GraphicsFrameStatistics statistics;

			void createDevice()
			{
				gpu::GpuDeviceDescriptor desc;
				desc.window = config.compatibility;
				device = gpu::newGpuDevice(desc);
			}

			GraphicsDeviceImpl(const GraphicsDeviceCreateConfig &config) : config(config)
			{
				createDevice();

				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing caches");
				bindingsCache = privat::newDeviceBindingsCache(this);
				pipelinesCache = privat::newDevicePipelinesCache(this);
				buffersCache = privat::newDeviceBuffersCache(this);
				texturesCache = privat::newDeviceTexturesCache(this);
				//gpuTimer = systemMemory().createHolder<GpuFrameTimer>(this);
				cpuTimer = newTimer();
			}

			~GraphicsDeviceImpl() {}

			void insertCommandBuffer(gpu::CommandBuffer &&cmds, const GraphicsCommandBufferStatistics &statistics_)
			{
				ScopeLock lock(mutex);
				commands.push_back(std::move(cmds));
				(GraphicsCommandBufferStatistics &)this->statistics = (GraphicsCommandBufferStatistics &)this->statistics + statistics_;
			}

			GraphicsFrameStatistics nextFrame(PointerRange<GraphicsWindowPresentation> windows)
			{
				const ProfilingScope profiling("next frame");
				//gpuTimer->frameEnd();
				GraphicsFrameStatistics stats;
				{
					const ProfilingScope profiling("queue submit");
					ankerl::svector<gpu::WindowPresentationDescriptor, 1> wpds;
					wpds.resize(windows.size());
					for (uint32 i = 0; i < windows.size(); i++)
						wpds[i].window = windows[i].window;
					{
						ScopeLock lock(mutex);
						device.submitAndPresentWindows(commands, wpds);
						commands.clear();
						std::swap(stats, statistics); // propagate statistics and clear
					}
					for (uint32 i = 0; i < windows.size(); i++)
					{
						gpu::Texture &t = wpds[i].texture;
						if (t)
						{
							gpu::TextureViewDescriptor tvd;
							tvd.dimension = gpu::TextureDimensionEnum::e2D;
							gpu::TextureView v = t.createView(tvd);
							windows[i].texture = newTexture(std::move(t), v, {}, "window surface texture");
						}
					}
				}
				//gpuTimer->frameStart();
				//stats.gpuTime = gpuTimer->time;
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
		/*
		GpuFrameTimer::GpuFrameTimer(GraphicsDeviceImpl *device) : device(device)
		{
			{
				gpu::QuerySetDescriptor qsDesc;
				qsDesc.type = gpu::QueryType::Timestamp;
				qsDesc.count = Frames * 2;
				querySet = device->device.CreateQuerySet(&qsDesc);
			}
			{
				gpu::BufferDescriptor resolveDesc;
				resolveDesc.size = Frames * 256; // alignment requirements
				resolveDesc.usage = gpu::BufferUsageFlags::QueryResolve | gpu::BufferUsageFlags::CopySrc;
				buffResolve = device->device.createBuffer(&resolveDesc);
			}
			for (uint32 i = 0; i < Frames; i++)
			{
				gpu::BufferDescriptor readbackDesc;
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
		*/
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

	Holder<gpu::Device> GraphicsDevice::nativeDevice()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		struct LockedDevice : private Noncopyable
		{
			ScopeLock<Mutex> lock;
			gpu::Device device;
			LockedDevice(ScopeLock<Mutex> &&l, gpu::Device d) : lock(std::move(l)), device(std::move(d)) {}
		};
		Holder<LockedDevice> l = systemMemory().createHolder<LockedDevice>(ScopeLock(impl->mutex), impl->device);
		return Holder<gpu::Device>(&l->device, std::move(l));
	}

	void GraphicsDevice::insertCommandBuffer(gpu::CommandBuffer &&commands, const GraphicsCommandBufferStatistics &statistics)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->insertCommandBuffer(std::move(commands), statistics);
	}

	GraphicsFrameStatistics GraphicsDevice::nextFrame(PointerRange<GraphicsWindowPresentation> windows)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->nextFrame(windows);
	}
}
