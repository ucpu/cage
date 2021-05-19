#ifndef guard_uniformBuffer_h_sdr5g4t5edr4g65e4rg6
#define guard_uniformBuffer_h_sdr5g4t5edr4g65e4rg6

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API UniformBuffer : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 id() const;
		void bind() const;
		void bind(uint32 bindingPoint) const;
		void bind(uint32 bindingPoint, uint32 offset, uint32 size) const;

		void writeWhole(PointerRange<const char> buffer, uint32 usage = 0);
		void writeRange(PointerRange<const char> buffer, uint32 offset);

		uint32 size() const;

		static uint32 alignmentRequirement();
	};

	struct CAGE_ENGINE_API UniformBufferCreateConfig
	{
		uint32 size = 0;
		bool persistentMapped = false, coherentMapped = false, explicitFlush = false;
	};

	CAGE_ENGINE_API Holder<UniformBuffer> newUniformBuffer(const UniformBufferCreateConfig &config = {});
}

#endif // guard_uniformBuffer_h_sdr5g4t5edr4g65e4rg6
