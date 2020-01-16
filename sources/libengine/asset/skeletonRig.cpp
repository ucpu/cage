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
			Holder<SkeletonRig> skl = newSkeletonRig();
			skl->setDebugName(context->textName);

			Deserializer des(context->originalData());
			SkeletonRigHeader data;
			des >> data;
			uint16 *boneParents = (uint16*)des.advance(sizeof(uint16) * data.bonesCount);
			mat4 *baseMatrices = (mat4*)des.advance(sizeof(mat4) * data.bonesCount);
			mat4 *invRestMatrices = (mat4*)des.advance(sizeof(mat4) * data.bonesCount);
			skl->allocate(data.globalInverse, data.bonesCount, boneParents, baseMatrices, invRestMatrices);
			CAGE_ASSERT(des.available() == 0);

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
