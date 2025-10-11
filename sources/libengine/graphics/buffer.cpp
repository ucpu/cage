#include <webgpu/webgpu_cpp.h>

#include <cage-engine/gpuBuffer.h>
#include <cage-engine/graphicsDevice.h>

namespace cage
{
	namespace
	{
		class GpuBufferImpl : public GpuBuffer
		{
		public:
			wgpu::Buffer buffer;
			GraphicsDevice *device = nullptr;

			explicit GpuBufferImpl(GraphicsDevice *device, wgpu::BufferUsage usage) : device(device)
			{
				wgpu::BufferDescriptor desc = {};
				desc.usage = usage | wgpu::BufferUsage::CopyDst;
				buffer = device->nativeDevice().CreateBuffer(&desc);
			}
		};
	}

	void GpuBuffer::setLabel(const String &name)
	{
		label = name;
		GpuBufferImpl *impl = (GpuBufferImpl *)this;
		impl->buffer.SetLabel(name.c_str());
	}

	void GpuBuffer::write(PointerRange<const char> buffer)
	{
		GpuBufferImpl *impl = (GpuBufferImpl *)this;
		wgpu::BufferDescriptor desc = {};
		desc.size = buffer.size();
		desc.usage = impl->buffer.GetUsage();
		impl->buffer = impl->device->nativeDevice().CreateBuffer(&desc);
		if (buffer.size() > 0)
			write(0, buffer);
	}

	void GpuBuffer::write(uint32 offset, PointerRange<const char> buffer)
	{
		CAGE_ASSERT(offset + buffer.size() <= size());
		GpuBufferImpl *impl = (GpuBufferImpl *)this;
		impl->device->nativeQueue()->WriteBuffer(impl->buffer, offset, buffer.data(), buffer.size());
	}

	uint32 GpuBuffer::size() const
	{
		const GpuBufferImpl *impl = (const GpuBufferImpl *)this;
		return numeric_cast<uint32>(impl->buffer.GetSize());
	}

	wgpu::Buffer GpuBuffer::nativeBuffer()
	{
		GpuBufferImpl *impl = (GpuBufferImpl *)this;
		return impl->buffer;
	}

	Holder<GpuBuffer> newGpuBufferUniform(GraphicsDevice *device)
	{
		return systemMemory().createImpl<GpuBuffer, GpuBufferImpl>(device, wgpu::BufferUsage::Uniform);
	}

	Holder<GpuBuffer> newGpuBufferGeometry(GraphicsDevice *device)
	{
		return systemMemory().createImpl<GpuBuffer, GpuBufferImpl>(device, wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Index);
	}
}
