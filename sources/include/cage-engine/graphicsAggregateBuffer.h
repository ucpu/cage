#ifndef guard_graphicsAggregateBuffer_asezuj4hr85sdtfz
#define guard_graphicsAggregateBuffer_asezuj4hr85sdtfz

#include <cage-engine/graphicsBindings.h>
#include <cage-engine/graphicsCommon.h>

namespace cage
{
	class GraphicsDevice;
	class GraphicsBuffer;

	struct AggregatedBinding : public GraphicsBindingsCreateConfig::BufferBindingConfig
	{
		uint32 dynamicOffset = m;

		operator uint32() const { return dynamicOffset; }
	};

	class CAGE_ENGINE_API GraphicsAggregateBuffer : private Immovable
	{
	public:
		[[nodiscard]] AggregatedBinding writeBuffer(PointerRange<const char> data, uint32 binding, bool uniform);

		template<class T>
		requires(privat::GraphicsBufferWritable<T>)
		[[nodiscard]] AggregatedBinding writeStruct(const T &data, uint32 binding, bool uniform)
		{
			return writeBuffer({ (const char *)&data, (const char *)(&data + 1) }, binding, uniform);
		}

		template<class T>
		requires(privat::GraphicsBufferWritable<T>)
		[[nodiscard]] AggregatedBinding writeArray(PointerRange<const T> data, uint32 binding, bool uniform)
		{
			return writeBuffer({ (const char *)data.data(), (const char *)(data.data() + data.size()) }, binding, uniform);
		}

		void submit();
	};

	struct CAGE_ENGINE_API GraphicsAggregateBufferCreateConfig
	{
		GraphicsDevice *device = nullptr;
	};

	CAGE_ENGINE_API Holder<GraphicsAggregateBuffer> newGraphicsAggregateBuffer(const GraphicsAggregateBufferCreateConfig &config);
}

#endif
