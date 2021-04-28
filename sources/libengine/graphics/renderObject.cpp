#include <cage-engine/renderObject.h>
#include "private.h"

#include <vector>
#include <algorithm>

namespace cage
{
	namespace
	{
		class RenderObjectImpl : public RenderObject
		{
		public:
			std::vector<float> thresholds;
			std::vector<uint32> indices;
			std::vector<uint32> names;
		};
	}

	void RenderObject::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void RenderObject::setLods(PointerRange<const real> thresholds, PointerRange<const uint32> modelIndices, PointerRange<const uint32> modelNames)
	{
		CAGE_ASSERT(modelIndices[0] == 0);
		CAGE_ASSERT(modelIndices.size() == thresholds.size() + 1);
		CAGE_ASSERT(modelIndices[thresholds.size()] == modelNames.size());
		CAGE_ASSERT(std::is_sorted(thresholds.begin(), thresholds.end(), [](real a, real b) {
			return b < a;
		}));
		CAGE_ASSERT(std::is_sorted(modelIndices.begin(), modelIndices.end()));
		RenderObjectImpl *impl = (RenderObjectImpl*)this;
		impl->thresholds.resize(thresholds.size());
		impl->indices.resize(thresholds.size() + 1);
		impl->names.resize(modelNames.size());
		detail::memcpy(impl->thresholds.data(), thresholds.data(), sizeof(uint32) * thresholds.size());
		detail::memcpy(impl->indices.data(), modelIndices.data(), sizeof(uint32) * (thresholds.size() + 1));
		detail::memcpy(impl->names.data(), modelNames.data(), sizeof(float) * modelNames.size());
	}

	uint32 RenderObject::lodsCount() const
	{
		RenderObjectImpl *impl = (RenderObjectImpl*)this;
		return numeric_cast<uint32>(impl->thresholds.size());
	}

	uint32 RenderObject::lodSelect(real threshold) const
	{
		RenderObjectImpl *impl = (RenderObjectImpl*)this;
		// todo rewrite to binary search
		uint32 cnt = numeric_cast<uint32>(impl->thresholds.size());
		uint32 lod = 0;
		while (lod + 1 < cnt && threshold < impl->thresholds[lod + 1])
			lod++;
		return lod;
	}

	PointerRange<const uint32> RenderObject::models(uint32 lod) const
	{
		RenderObjectImpl *impl = (RenderObjectImpl*)this;
		CAGE_ASSERT(lod < lodsCount());
		return { impl->names.data() + impl->indices[lod], impl->names.data() + impl->indices[lod + 1] };
	}

	Holder<RenderObject> newRenderObject()
	{
		return systemArena().createImpl<RenderObject, RenderObjectImpl>();
	}
}
