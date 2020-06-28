#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>

#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const AssetContext *context)
		{
			Holder<SkeletalAnimation> ani = newSkeletalAnimation();
			ani->setDebugName(context->textName);

			Deserializer des(context->originalData());
			SkeletalAnimationHeader data;
			des >> data;
			ani->duration = data.duration;
			ani->deserialize(data.animationBonesCount, des.advance(des.available()));

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
