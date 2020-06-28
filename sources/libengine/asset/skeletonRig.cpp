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
			Holder<SkeletonRig> skl = newSkeletonRig();
			skl->setDebugName(context->textName);

			Deserializer des(context->originalData());
			SkeletonRigHeader data;
			des >> data;
			skl->deserialize(data.globalInverse, data.bonesCount, des.advance(des.available()));

			context->assetHolder = templates::move(skl).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSkeletonRig()
	{
		AssetScheme s;
		s.load.bind<&processLoad>();
		return s;
	}
}
