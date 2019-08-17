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
			skeletonRig *skl = nullptr;
			if (context->assetHolder)
			{
				skl = static_cast<skeletonRig*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSkeletonRig().cast<void>();
				skl = static_cast<skeletonRig*>(context->assetHolder.get());
				skl->setDebugName(context->textName);
			}
			context->returnData = skl;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			skeletonRigHeader data;
			des >> data;
			uint16 *boneParents = (uint16*)des.advance(sizeof(uint16) * data.bonesCount);
			mat4 *baseMatrices = (mat4*)des.advance(sizeof(mat4) * data.bonesCount);
			mat4 *invRestMatrices = (mat4*)des.advance(sizeof(mat4) * data.bonesCount);
			skl->allocate(data.globalInverse, data.bonesCount, boneParents, baseMatrices, invRestMatrices);
			CAGE_ASSERT(des.available() == 0);
		}
	}

	assetScheme genAssetSchemeSkeletonRig(uint32 threadIndex)
	{
		assetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		return s;
	}
}
