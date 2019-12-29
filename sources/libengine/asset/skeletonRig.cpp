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
			SkeletonRig *skl = nullptr;
			if (context->assetHolder)
			{
				skl = static_cast<SkeletonRig*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSkeletonRig().cast<void>();
				skl = static_cast<SkeletonRig*>(context->assetHolder.get());
				skl->setDebugName(context->textName);
			}
			context->returnData = skl;

			Deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			SkeletonRigHeader data;
			des >> data;
			uint16 *boneParents = (uint16*)des.advance(sizeof(uint16) * data.bonesCount);
			mat4 *baseMatrices = (mat4*)des.advance(sizeof(mat4) * data.bonesCount);
			mat4 *invRestMatrices = (mat4*)des.advance(sizeof(mat4) * data.bonesCount);
			skl->allocate(data.globalInverse, data.bonesCount, boneParents, baseMatrices, invRestMatrices);
			CAGE_ASSERT(des.available() == 0);
		}
	}

	AssetScheme genAssetSchemeSkeletonRig(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		return s;
	}
}
