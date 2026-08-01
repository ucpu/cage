#include <svector.h>
#include <vector>

#include <cage-core/concurrent.h>
#include <cage-core/files.h>
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
		class GraphicsDeviceImpl;

		class GpuFrameTimer : private Immovable
		{
		public:
			GpuFrameTimer(GraphicsDeviceImpl *device);

			void frameStart();
			void frameEnd();

			double conversion = 0;
			uint64 time = 0; // microseconds

		private:
			static constexpr uint32 Frames = 2;
			GraphicsDeviceImpl *device = nullptr;
			gpu::QuerySet querySet;
			gpu::Buffer buffResolve;
			std::array<gpu::Buffer, Frames> buffRead = {};
			uint32 frameIndex = 0;
		};
	}

	namespace
	{
		class GraphicsDeviceImpl : public GraphicsDevice
		{
		public:
			Holder<Mutex> mutex = newMutex(); // used for commands and statistics
			GraphicsDeviceCreateConfig config;
			gpu::Device device;
			Holder<privat::DeviceBindingsCache> bindingsCache;
			Holder<privat::DevicePipelinesCache> pipelinesCache;
			Holder<privat::DeviceBuffersCache> buffersCache;
			Holder<privat::DeviceTexturesCache> texturesCache;
			std::vector<gpu::CommandBuffer> commands;
			Holder<GpuFrameTimer> gpuTimer;
			Holder<Timer> cpuTimer;
			GraphicsFrameStatistics statistics;

			GraphicsDeviceImpl(const GraphicsDeviceCreateConfig &config) : config(config)
			{
				gpu::GpuDeviceDescriptor desc;
				desc.label = pathExtractFilenameNoExtension(detail::pathExecutable());
				desc.window = config.compatibility;
				device = gpu::newGpuDevice(desc);

				CAGE_LOG(SeverityEnum::Info, "graphics", "initializing caches");
				bindingsCache = privat::newDeviceBindingsCache(this);
				pipelinesCache = privat::newDevicePipelinesCache(this);
				buffersCache = privat::newDeviceBuffersCache(this);
				texturesCache = privat::newDeviceTexturesCache(this);
				gpuTimer = systemMemory().createHolder<GpuFrameTimer>(this);
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
				gpuTimer->frameEnd();
				GraphicsFrameStatistics stats;
				{
					const ProfilingScope profiling("queue submit");
					ankerl::svector<gpu::WindowPresentationDescriptor, 1> wpds;
					wpds.resize(windows.size());
					for (uint32 i = 0; i < windows.size(); i++)
						wpds[i].window = windows[i].window;
					{
						ScopeLock lock(mutex);
						device.submitAndPresent(commands, wpds);
						commands.clear();
						std::swap(stats, statistics); // propagate statistics and clear
					}
					for (uint32 i = 0; i < windows.size(); i++)
					{
						gpu::Texture &t = wpds[i].texture;
						if (t)
						{
							gpu::TextureViewDescriptor tvd;
							tvd.label = "window surface view";
							tvd.dimension = gpu::TextureDimensionEnum::e2D;
							gpu::TextureView v = t.createView(tvd);
							windows[i].texture = newTexture(std::move(t), v, {}, "window surface texture");
						}
					}
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
			conversion = device->device.getTimestampsConversion();
			{
				gpu::QuerySetDescriptor qsDesc;
				qsDesc.label = "frame timing";
				qsDesc.count = Frames * 2;
				querySet = device->device.createQuerySet(qsDesc);
			}
			{
				gpu::BufferDescriptor resolveDesc;
				resolveDesc.label = "frame timing resolve";
				resolveDesc.size = Frames * 256; // alignment requirements
				resolveDesc.usage = gpu::BufferUsageFlags::CopyDst | gpu::BufferUsageFlags::CopySrc;
				buffResolve = device->device.createBuffer(resolveDesc);
			}
			for (uint32 i = 0; i < Frames; i++)
			{
				gpu::BufferDescriptor readbackDesc;
				readbackDesc.label = "frame timing readback";
				readbackDesc.size = 2 * sizeof(uint64);
				readbackDesc.usage = gpu::BufferUsageFlags::MapRead | gpu::BufferUsageFlags::CopyDst;
				buffRead[i] = device->device.createBuffer(readbackDesc);
			}
		}

		void GpuFrameTimer::frameStart()
		{
			const ProfilingScope profiling("gpu timer start");
			const uint32 current = frameIndex % Frames;
			auto ce = device->device.createCommandEncoder({ .label = "frame timing start" });
			ce.writeTimestamp(querySet, current * 2 + 0);
			device->insertCommandBuffer(ce.finishEncoding(), {});
		}

		void GpuFrameTimer::frameEnd()
		{
			const ProfilingScope profiling("gpu timer end");
			{
				const uint32 current = frameIndex % Frames;
				const uint64 offset = current * 256;
				auto ce = device->device.createCommandEncoder({ .label = "frame timing end" });
				ce.writeTimestamp(querySet, current * 2 + 1);
				if (frameIndex >= Frames)
				{
					ce.resolveQuerySet(querySet, current * 2, 2, buffResolve, offset);
					ce.copyBufferToBuffer(buffResolve, offset, buffRead[current], 0, 2 * sizeof(uint64));
				}
				device->insertCommandBuffer(ce.finishEncoding(), {}); // this enques the cmdbuf to be submitted, but does not submit it yet
			}
			if (frameIndex >= Frames * 2)
			{
				const uint32 next = (frameIndex + 1) % Frames;
				const uint64 *ts = (uint64 *)(buffRead[next].getMappedRange().data());
				time = (ts[1] - ts[0]) * conversion * 0.001; // ns -> us
				ProfilingEvent ev = profilingEventBegin("gpu", ProfilingFrameTag());
				profilingEventEnd(ev, time);
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

	gpu::Device *GraphicsDevice::nativeDevice()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return &impl->device;
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
