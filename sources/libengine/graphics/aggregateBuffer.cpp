#include <array>
#include <atomic>
#include <vector>

#include <svector.h>

#include <cage-core/concurrent.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/serialization.h>
#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>

namespace cage
{
	namespace privat
	{
		struct DeviceBuffersCache : private Immovable
		{
		public:
			Holder<Mutex> mutex = newMutex();
			GraphicsDevice *device = nullptr;

			struct Cache
			{
				Holder<GraphicsBuffer> buffer;
				uint32 frameIndex = 0;

				Cache(GraphicsDevice *device) { buffer = newGraphicsBufferStorage(device, 2'000'000, "transient buffer"); }
			};
			std::vector<Holder<Cache>> available, waiting;

			uint32 currentFrame = 1;
			std::atomic<uint32> finishedFrame = 0;
			uint32 createdBuffers = 0;

			DeviceBuffersCache(GraphicsDevice *device) : device(device) {}

			~DeviceBuffersCache() { CAGE_LOG_DEBUG(SeverityEnum::Info, "graphics", Stringizer() + "graphics buffers cache size: " + createdBuffers); }

			Holder<Cache> getBuffer()
			{
				ScopeLock lock(mutex);

				if (available.empty())
				{
					createdBuffers++;
					return systemMemory().createHolder<Cache>(device);
				}

				Holder<Cache> c = std::move(available.back());
				available.pop_back();
				return c;
			}

			void returnBuffer(Holder<Cache> &&cache)
			{
				ScopeLock lock(mutex);
				cache->frameIndex = currentFrame;
				waiting.push_back(std::move(cache));
			}

			void nextFrame()
			{
				ScopeLock lock(mutex);

				for (auto &it : waiting)
				{
					if (it && it->frameIndex <= finishedFrame)
						available.push_back(std::move(it));
				}
				std::erase_if(waiting, [](auto &it) { return !it; });

				device->nativeQueue()->OnSubmittedWorkDone(wgpu::CallbackMode::AllowSpontaneous, // AllowProcessEvents is preffered but never triggers the callback
					[&, expected = currentFrame](wgpu::QueueWorkDoneStatus status, wgpu::StringView message)
					{
						if (status == wgpu::QueueWorkDoneStatus::Success)
							finishedFrame = max(finishedFrame, expected);
					});
			}
		};

		DeviceBuffersCache *getDeviceBuffersCache(GraphicsDevice *device);

		Holder<DeviceBuffersCache> newDeviceBuffersCache(GraphicsDevice *device)
		{
			return systemMemory().createHolder<DeviceBuffersCache>(device);
		}

		void deviceCacheNextFrame(DeviceBuffersCache *cache)
		{
			cache->nextFrame();
		}
	}

	namespace
	{
		class GraphicsAggregateBufferImpl : public GraphicsAggregateBuffer
		{
		public:
			GraphicsAggregateBufferCreateConfig config;
			Holder<privat::DeviceBuffersCache::Cache> cache;
			MemoryBuffer memory;
			Serializer ser;
			uintPtr capacity = 0;

			GraphicsAggregateBufferImpl(const GraphicsAggregateBufferCreateConfig &config) : config(config), ser(memory) {}

			~GraphicsAggregateBufferImpl()
			{
				submit();
				if (cache)
					privat::getDeviceBuffersCache(config.device)->returnBuffer(std::move(cache));
			}

			void reset()
			{
				CAGE_ASSERT(!cache);
				cache = privat::getDeviceBuffersCache(config.device)->getBuffer();
				capacity = cache->buffer->size();
			}

			void fill(uint32 sz)
			{
				if (sz == 0)
					return;
				static constexpr std::array<char, 256> nothing = {};
				ser.write(PointerRange<const char>(nothing).subRange(0, sz));
			}

			AggregatedBinding writeBuffer(PointerRange<const char> data, uint32 binding)
			{
				if (memory.size() + detail::roundUpTo(data.size(), 256) > capacity)
				{
					submit();
					reset();
				}
				CAGE_ASSERT(cache && cache->buffer);
				AggregatedBinding res;
				res.buffer = +cache->buffer;
				res.binding = binding;
				res.size = data.size();
				res.dynamic = true;
				res.dynamicOffset = memory.size();
				ser.write(data);
				fill(detail::addToAlign(data.size(), 256));
				CAGE_ASSERT(memory.size() <= capacity);
				return res;
			}

			void submit()
			{
				if (!cache)
					return;
				if (memory.size() > 0)
				{
					cache->buffer->writeBuffer(PointerRange<const char>(memory));
					memory.clear();
					ser = Serializer(memory);
				}
				privat::getDeviceBuffersCache(config.device)->returnBuffer(std::move(cache));
				capacity = 0;
			}
		};
	}

	AggregatedBinding GraphicsAggregateBuffer::writeBuffer(PointerRange<const char> data, uint32 binding)
	{
		GraphicsAggregateBufferImpl *impl = (GraphicsAggregateBufferImpl *)this;
		return impl->writeBuffer(data, binding);
	}

	void GraphicsAggregateBuffer::submit()
	{
		GraphicsAggregateBufferImpl *impl = (GraphicsAggregateBufferImpl *)this;
		impl->submit();
	}

	Holder<GraphicsAggregateBuffer> newGraphicsAggregateBuffer(const GraphicsAggregateBufferCreateConfig &config)
	{
		return systemMemory().createImpl<GraphicsAggregateBuffer, GraphicsAggregateBufferImpl>(config);
	}
}
