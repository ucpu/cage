#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
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
		void processLoad(const assetContext *context, void *schemePointer)
		{
			if (!context->assetHolder)
			{
				context->assetHolder = newObject().cast<void>();
				static_cast<objectClass*>(context->assetHolder.get())->setDebugName(context->textName);
			}
			objectClass *obj = static_cast<objectClass*>(context->assetHolder.get());
			context->returnData = obj;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			objectHeaderStruct h;
			des >> h;
			obj->worldSize = h.worldSize;
			obj->pixelsSize = h.pixelsSize;

			float *thresholds = (float*)des.advance(h.lodsCount * sizeof(uint32));
			uint32 *indices = (uint32*)des.advance((h.lodsCount + 1) * sizeof(uint32));
			uint32 *names = (uint32*)des.advance(h.meshesCount * sizeof(uint32));
			obj->setLods(h.lodsCount, h.meshesCount, thresholds, indices, names);
		}
	}

	assetScheme genAssetSchemeObject(uint32 threadIndex)
	{
		assetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		return s;
	}
}
