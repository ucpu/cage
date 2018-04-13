#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#define CAGE_EXPORT

#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include "private.h"

namespace cage
{
	namespace
	{
		struct lodStruct
		{
			lodStruct() : threshold(0)
			{}

			float threshold;
			std::vector<uint32> meshes;
		};

		class objectImpl : public objectClass
		{
		public:
			objectImpl()
			{
				shadower = 0;
				collider = 0;
				worldSize = 0;
				pixelsSize = 0;
			}

			std::vector<lodStruct> lods;
		};
	}

	void objectClass::setLodLevels(uint32 count)
	{
		objectImpl *impl = (objectImpl*)this;
		impl->lods.resize(count);
	}

	void objectClass::setLodThreshold(uint32 lod, float threshold)
	{
		objectImpl *impl = (objectImpl*)this;
		impl->lods[lod].threshold = threshold;
	}

	void objectClass::setLodMeshes(uint32 lod, uint32 count)
	{
		objectImpl *impl = (objectImpl*)this;
		impl->lods[lod].meshes.resize(count);
	}

	void objectClass::setMeshName(uint32 lod, uint32 index, uint32 name)
	{
		objectImpl *impl = (objectImpl*)this;
		impl->lods[lod].meshes[index] = name;
	}

	uint32 objectClass::lodsCount() const
	{
		objectImpl *impl = (objectImpl*)this;
		return numeric_cast<uint32>(impl->lods.size());
	}

	real  objectClass::lodsThreshold(uint32 lod) const
	{
		objectImpl *impl = (objectImpl*)this;
		return impl->lods[lod].threshold;
	}

	uint32 objectClass::meshesCount(uint32 lod) const
	{
		objectImpl *impl = (objectImpl*)this;
		return numeric_cast<uint32>(impl->lods[lod].meshes.size());
	}

	uint32 objectClass::meshesName(uint32 lod, uint32 index) const
	{
		objectImpl *impl = (objectImpl*)this;
		return impl->lods[lod].meshes[index];
	}

	pointerRange<const uint32> objectClass::meshes(uint32 lod) const
	{
		objectImpl *impl = (objectImpl*)this;
		return impl->lods[lod].meshes;
	}

	holder<objectClass> newObject()
	{
		return detail::systemArena().createImpl<objectClass, objectImpl>();
	}
}