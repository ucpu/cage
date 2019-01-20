#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include <cage-core/serialization.h>
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

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			objectHeaderStruct h;
			des >> h;
			obj->worldSize = h.worldSize;
			obj->pixelsSize = h.pixelsSize;

			float *thresholds = (float*)des.access(h.lodsCount * sizeof(uint32));
			uint32 *indices = (uint32*)des.access((h.lodsCount + 1) * sizeof(uint32));
			uint32 *names = (uint32*)des.access(h.meshesCount * sizeof(uint32));
			obj->setLods(h.lodsCount, h.meshesCount, thresholds, indices, names);
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
