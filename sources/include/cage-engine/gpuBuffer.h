#ifndef guard_gpuBuffer_s54dtu4kfesas
#define guard_gpuBuffer_s54dtu4kfesas

#include <cage-engine/core.h>

namespace wgpu
{
	class Buffer;
}

namespace cage
{
	class GraphicsDevice;

	class CAGE_ENGINE_API GpuBuffer : private Immovable
	{
	protected:
		detail::StringBase<128> label;

	public:
		void setLabel(const String &name);

		void write(PointerRange<const char> buffer);
		void write(uint32 offset, PointerRange<const char> buffer);

		uint32 size() const;

		wgpu::Buffer nativeBuffer();
	};

	CAGE_ENGINE_API Holder<GpuBuffer> newGpuBufferUniform(GraphicsDevice *device);
	CAGE_ENGINE_API Holder<GpuBuffer> newGpuBufferGeometry(GraphicsDevice *device);
}

#endif // guard_gpuBuffer_s54dtu4kfesas
