#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const AssetContext *context, void *schemePointer)
		{
			Holder<SkeletalAnimation> ani = newSkeletalAnimation();
			ani->setDebugName(context->textName);

			Deserializer des(context->originalData());
			SkeletalAnimationHeader data;
			des >> data;
			uint16 *indexes = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			uint16 *positionFrames = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			uint16 *rotationFrames = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			uint16 *scaleFrames = (uint16*)des.advance(data.animationBonesCount * sizeof(uint16));
			ani->allocate(data.duration, data.animationBonesCount, indexes, positionFrames, rotationFrames, scaleFrames, des.advance(0));

			context->assetHolder = templates::move(ani).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSkeletalAnimation()
	{
		AssetScheme s;
		s.load.bind<&processLoad>();
		return s;
	}
}
