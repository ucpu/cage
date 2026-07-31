#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>

namespace cage
{
	namespace
	{
		class GraphicsBufferImpl : public GraphicsBuffer
		{
		public:
			gpu::Buffer buffer;
			GraphicsDevice *device = nullptr;
			uint64 size = 0;
			bool mapped = false;

			explicit GraphicsBufferImpl(GraphicsDevice *device, uint64 size, const AssetLabel &label_, uint32 type) : device(device), size(size), mapped(type == 1)
			{
				this->label = label_;

				CAGE_ASSERT((size % 4) == 0);

				if (type != 2) // not geometry
					size = ((max(size, uint64(256)) + 15) / 16) * 16;
				gpu::BufferDescriptor desc;
				desc.size = size;
				desc.usage = usage(type);
				desc.label = label;

				buffer = device->nativeDevice()->createBuffer(desc);
			}

			gpu::BufferUsageFlags usage(uint32 type) const
			{
				switch (type)
				{
					case 0: // uniform/storage
						return gpu::BufferUsageFlags::Uniform | gpu::BufferUsageFlags::Storage | gpu::BufferUsageFlags::CopyDst;
					case 1: // mapped
						return gpu::BufferUsageFlags::Uniform | gpu::BufferUsageFlags::Storage | gpu::BufferUsageFlags::MapWrite;
					case 2: // geometry
						return gpu::BufferUsageFlags::GeometryVertex | gpu::BufferUsageFlags::GeometryIndex | gpu::BufferUsageFlags::CopyDst;
					default:
						return gpu::BufferUsageFlags::Undefined;
				}
			}
		};
	}

	void GraphicsBuffer::writeBuffer(PointerRange<const char> buffer, uint64 offset)
	{
		GraphicsBufferImpl *impl = (GraphicsBufferImpl *)this;

		CAGE_ASSERT((offset % 4) == 0);
		CAGE_ASSERT((buffer.size() % 4) == 0);
		CAGE_ASSERT(offset + buffer.size() <= impl->size);

		if (impl->mapped)
		{
			const auto r = impl->nativeBuffer().getMappedRange();
			detail::memcpy(r.data() + offset, buffer.data(), buffer.size());
		}
		else
			impl->device->nativeDevice()->writeBuffer(impl->buffer, offset, buffer);
	}

	uint64 GraphicsBuffer::size() const
	{
		const GraphicsBufferImpl *impl = (const GraphicsBufferImpl *)this;
		return numeric_cast<uint32>(impl->buffer.getSize());
	}

	const gpu::Buffer &GraphicsBuffer::nativeBuffer()
	{
		GraphicsBufferImpl *impl = (GraphicsBufferImpl *)this;
		return impl->buffer;
	}

	Holder<GraphicsBuffer> newGraphicsBuffer(GraphicsDevice *device, uint64 size, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsBuffer, GraphicsBufferImpl>(device, size, label, 0);
	}

	Holder<GraphicsBuffer> newGraphicsBufferMapped(GraphicsDevice *device, uint64 size, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsBuffer, GraphicsBufferImpl>(device, size, label, 1);
	}

	Holder<GraphicsBuffer> newGraphicsBufferGeometry(GraphicsDevice *device, uint64 size, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsBuffer, GraphicsBufferImpl>(device, size, label, 2);
	}
}
