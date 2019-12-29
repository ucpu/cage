#include <vector>
#include <algorithm>

#include <cage-core/core.h>
#include <cage-core/math.h>
#define CAGE_EXPORT

#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class objectImpl : public renderObject
		{
		public:
			std::vector<float> thresholds;
			std::vector<uint32> indices;
			std::vector<uint32> names;
		};
	}

	renderObject::renderObject() : color(real::Nan()), opacity(real::Nan()), texAnimSpeed(real::Nan()), texAnimOffset(real::Nan()), skelAnimName(0), skelAnimSpeed(real::Nan()), skelAnimOffset(real::Nan())
	{}

	void renderObject::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void renderObject::setLods(uint32 lodsCount, uint32 meshesCount, const float *thresholds, const uint32 *meshIndices, const uint32 *meshNames)
	{
		CAGE_ASSERT(meshIndices[0] == 0);
		CAGE_ASSERT(meshIndices[lodsCount] == meshesCount);
		CAGE_ASSERT(std::is_sorted(thresholds, thresholds + lodsCount, [](float a, float b) {
			return b < a;
		}));
		CAGE_ASSERT(std::is_sorted(meshIndices, meshIndices + lodsCount + 1));
		objectImpl *impl = (objectImpl*)this;
		impl->thresholds.resize(lodsCount);
		impl->indices.resize(lodsCount + 1);
		impl->names.resize(meshesCount);
		detail::memcpy(impl->thresholds.data(), thresholds, sizeof(uint32) * lodsCount);
		detail::memcpy(impl->indices.data(), meshIndices, sizeof(uint32) * (lodsCount + 1));
		detail::memcpy(impl->names.data(), meshNames, sizeof(float) * meshesCount);
	}

	uint32 renderObject::lodsCount() const
	{
		objectImpl *impl = (objectImpl*)this;
		return numeric_cast<uint32>(impl->thresholds.size());
	}

	uint32 renderObject::lodSelect(float threshold) const
	{
		objectImpl *impl = (objectImpl*)this;
		// todo rewrite to binary search
		uint32 cnt = numeric_cast<uint32>(impl->thresholds.size());
		uint32 lod = 0;
		while (lod + 1 < cnt && threshold < impl->thresholds[lod + 1])
			lod++;
		return lod;
	}

	PointerRange<const uint32> renderObject::meshes(uint32 lod) const
	{
		objectImpl *impl = (objectImpl*)this;
		CAGE_ASSERT(lod < lodsCount());
		return { impl->names.data() + impl->indices[lod], impl->names.data() + impl->indices[lod + 1] };
	}

	Holder<renderObject> newRenderObject()
	{
		return detail::systemArena().createImpl<renderObject, objectImpl>();
	}
}
