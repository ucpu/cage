#include <vector>
#include <algorithm>

#include <cage-core/core.h>
#include <cage-core/math.h>
#define CAGE_EXPORT

#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include "private.h"

namespace cage
{
	namespace
	{
		class objectImpl : public objectClass
		{
		public:
			objectImpl()
			{
				worldSize = 0;
				pixelsSize = 0;
			}

			std::vector<float> thresholds;
			std::vector<uint32> indices;
			std::vector<uint32> names;
		};
	}

	void objectClass::setLods(uint32 lodsCount, uint32 meshesCount, const float *thresholds, const uint32 *meshIndices, const uint32 *meshNames)
	{
		CAGE_ASSERT_RUNTIME(meshIndices[0] == 0);
		CAGE_ASSERT_RUNTIME(meshIndices[lodsCount] == meshesCount);
		CAGE_ASSERT_RUNTIME(std::is_sorted(thresholds, thresholds + lodsCount, [](float a, float b) {
			return b < a;
		}));
		CAGE_ASSERT_RUNTIME(std::is_sorted(meshIndices, meshIndices + lodsCount + 1));
		objectImpl *impl = (objectImpl*)this;
		impl->thresholds.resize(lodsCount);
		impl->indices.resize(lodsCount + 1);
		impl->names.resize(meshesCount);
		detail::memcpy(impl->thresholds.data(), thresholds, sizeof(uint32) * lodsCount);
		detail::memcpy(impl->indices.data(), meshIndices, sizeof(uint32) * (lodsCount + 1));
		detail::memcpy(impl->names.data(), meshNames, sizeof(float) * meshesCount);
	}

	uint32 objectClass::lodsCount() const
	{
		objectImpl *impl = (objectImpl*)this;
		return numeric_cast<uint32>(impl->thresholds.size());
	}

	uint32 objectClass::lodSelect(float threshold) const
	{
		objectImpl *impl = (objectImpl*)this;
		// todo rewrite to binary search
		uint32 cnt = numeric_cast<uint32>(impl->thresholds.size());
		uint32 lod = 0;
		while (lod + 1 < cnt && threshold < impl->thresholds[lod + 1])
			lod++;
		return lod;
	}

	pointerRange<const uint32> objectClass::meshes(uint32 lod) const
	{
		objectImpl *impl = (objectImpl*)this;
		CAGE_ASSERT_RUNTIME(lod < lodsCount());
		return { impl->names.data() + impl->indices[lod], impl->names.data() + impl->indices[lod + 1] };
	}

	holder<objectClass> newObject()
	{
		return detail::systemArena().createImpl<objectClass, objectImpl>();
	}
}
