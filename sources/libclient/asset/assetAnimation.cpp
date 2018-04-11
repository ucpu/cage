#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include <cage-core/utility/pointer.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
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
				ani = static_cast<animationClass*> (context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newAnimation().transfev();
				ani = static_cast<animationClass*> (context->assetHolder.get());
			}
			context->returnData = ani;

			animationHeaderStruct *data = (animationHeaderStruct*)context->originalData;
			pointer ptr = pointer(context->originalData) + sizeof(animationHeaderStruct);
			const uint16 *indexes = (uint16*)ptr.asVoid;
			ptr += data->bonesCount * sizeof(uint16);
			const uint16 *positionFrames = (uint16*)ptr.asVoid;
			ptr += data->bonesCount * sizeof(uint16);
			const uint16 *rotationFrames = (uint16*)ptr.asVoid;
			ptr += data->bonesCount * sizeof(uint16);
			ani->allocate(data->duration, data->bonesCount, indexes, positionFrames, rotationFrames, ptr);
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
		s.load.bind <&processLoad>();
		s.done.bind <&processDone>();
		return s;
	}
}