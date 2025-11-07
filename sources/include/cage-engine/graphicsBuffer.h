#ifndef guard_graphicsBuffer_s54dtau4kfesas
#define guard_graphicsBuffer_s54dtau4kfesas

#include <cage-engine/graphicsCommon.h>

namespace wgpu
{
	class Buffer;
}

namespace cage
{
	class GraphicsDevice;

	class CAGE_ENGINE_API GraphicsBuffer : private Immovable
	{
	protected:
		AssetLabel label;

	public:
		void writeBuffer(PointerRange<const char> buffer, uintPtr offset = 0);

		template<class T>
		requires(privat::GraphicsBufferWritable<T>)
		void writeStruct(const T &data, uintPtr offset = 0)
		{
			return writeBuffer({ (const char *)&data, (const char *)(&data + 1) }, offset);
		}

		template<class T>
		requires(privat::GraphicsBufferWritable<T>)
		void writeArray(PointerRange<const T> data, uintPtr offset = 0)
		{
			return writeBuffer({ (const char *)data.data(), (const char *)(data.data() + data.size()) }, offset);
		}

		uintPtr size() const;
		uint32 type() const;

		const wgpu::Buffer &nativeBuffer();
	};

	CAGE_ENGINE_API Holder<GraphicsBuffer> newGraphicsBufferUniform(GraphicsDevice *device, uintPtr size, const AssetLabel &label);
	CAGE_ENGINE_API Holder<GraphicsBuffer> newGraphicsBufferStorage(GraphicsDevice *device, uintPtr size, const AssetLabel &label);
	CAGE_ENGINE_API Holder<GraphicsBuffer> newGraphicsBufferGeometry(GraphicsDevice *device, uintPtr size, const AssetLabel &label);
}

#endif // guard_gpuBuffer_s54dtu4kfesas
