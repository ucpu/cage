#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/assets.h>
#include <cage-client/assetStructures.h>

namespace cage
{
	namespace
	{
		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newObject().transfev();
			objectClass *obj = static_cast<objectClass*>(context->assetHolder.get());
			context->returnData = obj;

			objectHeaderStruct h;
			detail::memcpy(&h, context->originalData, sizeof(h));
			obj->collider = h.collider;
			obj->shadower = h.shadower;
			obj->worldSize = h.worldSize;
			obj->pixelsSize = h.pixelsSize;

			pointer p = context->originalData;
			p += sizeof(h);
			obj->setLodLevels(h.lodsCount);
			for (uint32 l = 0; l < h.lodsCount; l++)
			{
				float thr = *(float*)p.asVoid;
				p += sizeof(float); // threshold
				obj->setLodThreshold(l, thr);
				uint32 cnt = *(uint32*)p.asVoid;
				p += sizeof(uint32); // cnt
				obj->setLodMeshes(l, cnt);
				for (uint32 m = 0; m < cnt; m++)
				{
					uint32 nam = *(uint32*)p.asVoid;
					p += sizeof(uint32); // mesh name
					obj->setMeshName(l, m, nam);
				}
			}
			CAGE_ASSERT_RUNTIME(p == pointer(context->originalData) + (uintPtr)context->originalSize);
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeObject(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		return s;
	}
}
