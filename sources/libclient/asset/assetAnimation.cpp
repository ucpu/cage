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
			skeletalAnimation *ani = nullptr;
			if (context->assetHolder)
			{
				ani = static_cast<skeletalAnimation*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSkeletalAnimation().cast<void>();
				ani = static_cast<skeletalAnimation*>(context->assetHolder.get());
				ani->setDebugName(context->textName);
			}
			context->returnData = ani;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			skeletalAnimationHeader data;
			des >> data;
			uint16 *indexes = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			uint16 *positionFrames = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			uint16 *rotationFrames = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			uint16 *scaleFrames = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			ani->allocate(data.duration, data.animationBonesCount, indexes, positionFrames, rotationFrames, scaleFrames, des.advance(0));
		}
	}

	assetScheme genAssetSchemeSkeletalAnimation(uint32 threadIndex)
	{
		assetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		return s;
	}
}
