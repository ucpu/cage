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
			animationClass *ani = nullptr;
			if (context->assetHolder)
			{
				ani = static_cast<animationClass*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newAnimation().transfev();
				ani = static_cast<animationClass*>(context->assetHolder.get());
			}
			context->returnData = ani;

			deserializer des(context->originalData, context->originalSize);
			animationHeaderStruct data;
			des >> data;
			uint16 *indexes = (uint16*)des.access(data.animationBonesCount * sizeof(uint16));
			uint16 *positionFrames = (uint16*)des.access(data.animationBonesCount * sizeof(uint16));
			uint16 *rotationFrames = (uint16*)des.access(data.animationBonesCount * sizeof(uint16));
			uint16 *scaleFrames = (uint16*)des.access(data.animationBonesCount * sizeof(uint16));
			ani->allocate(data.duration, data.animationBonesCount, indexes, positionFrames, rotationFrames, scaleFrames, des.current);
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeAnimation(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		return s;
	}
}
