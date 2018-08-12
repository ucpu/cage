#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include <cage-core/utility/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/assetStructs.h>

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

			deserializer des(context->originalData, context->originalSize);
			objectHeaderStruct h;
			des >> h;
			obj->collider = h.collider;
			obj->shadower = h.shadower;
			obj->worldSize = h.worldSize;
			obj->pixelsSize = h.pixelsSize;
			obj->setLodLevels(h.lodsCount);

			for (uint32 l = 0; l < h.lodsCount; l++)
			{
				float thr;
				des >> thr;
				obj->setLodThreshold(l, thr);
				uint32 cnt;
				des >> cnt;
				obj->setLodMeshes(l, cnt);
				for (uint32 m = 0; m < cnt; m++)
				{
					uint32 nam;
					des >> nam;
					obj->setMeshName(l, m, nam);
				}
			}
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
