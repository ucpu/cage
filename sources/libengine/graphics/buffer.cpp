#include <webgpu/webgpu_cpp.h>

#include <cage-core/profiling.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>

namespace cage
{
	namespace
	{
		class GraphicsBufferImpl : public GraphicsBuffer
		{
		public:
			wgpu::Buffer buffer = {};
			GraphicsDevice *device = nullptr;
			uintPtr size = 0;
			bool geometry = false;

			explicit GraphicsBufferImpl(GraphicsDevice *device, uintPtr size, const AssetLabel &label_, bool geometry) : device(device), size(size), geometry(geometry)
			{
				this->label = label_;

				CAGE_ASSERT((size % 4) == 0);

				if (!geometry)
					size = ((max(size, uintPtr(256)) + 15) / 16) * 16;
				wgpu::BufferDescriptor desc = {};
				desc.size = size;
				desc.usage = usage();
				desc.label = label.c_str();

				const ProfilingScope profiling("buffer create");
				buffer = device->nativeDevice()->CreateBuffer(&desc);
			}

			wgpu::BufferUsage usage() const
			{
				if (geometry)
					return wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
				else
					return wgpu::BufferUsage::Uniform | wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
			}
		};
	}

	void GraphicsBuffer::writeBuffer(PointerRange<const char> buffer, uintPtr offset)
	{
		GraphicsBufferImpl *impl = (GraphicsBufferImpl *)this;

		CAGE_ASSERT((offset % 4) == 0);
		CAGE_ASSERT((buffer.size() % 4) == 0);
		CAGE_ASSERT(offset + buffer.size() <= impl->size);

		const ProfilingScope profiling("buffer write");
		impl->device->nativeQueue()->WriteBuffer(impl->buffer, offset, buffer.data(), buffer.size());
	}

	uintPtr GraphicsBuffer::size() const
	{
		const GraphicsBufferImpl *impl = (const GraphicsBufferImpl *)this;
		return numeric_cast<uint32>(impl->buffer.GetSize());
	}

	const wgpu::Buffer &GraphicsBuffer::nativeBuffer()
	{
		GraphicsBufferImpl *impl = (GraphicsBufferImpl *)this;
		return impl->buffer;
	}

	Holder<GraphicsBuffer> newGraphicsBuffer(GraphicsDevice *device, uintPtr size, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsBuffer, GraphicsBufferImpl>(device, size, label, false);
	}

	Holder<GraphicsBuffer> newGraphicsBufferGeometry(GraphicsDevice *device, uintPtr size, const AssetLabel &label)
	{
		return systemMemory().createImpl<GraphicsBuffer, GraphicsBufferImpl>(device, size, label, true);
	}
}
