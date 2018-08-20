#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include<cage-core/utility/serialization.h>
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
			skeletonClass *skl = nullptr;
			if (context->assetHolder)
			{
				skl = static_cast<skeletonClass*>(context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSkeleton().transfev();
				skl = static_cast<skeletonClass*>(context->assetHolder.get());
			}
			context->returnData = skl;

			deserializer des(context->originalData, context->originalSize);
			skeletonHeaderStruct data;
			des >> data;
			uint16 *boneParents = (uint16*)des.access(sizeof(uint16) * data.bonesCount);
			mat4 *baseMatrices = (mat4*)des.access(sizeof(mat4) * data.bonesCount);
			mat4 *invRestMatrices = (mat4*)des.access(sizeof(mat4) * data.bonesCount);
			skl->allocate(data.bonesCount, boneParents, baseMatrices, invRestMatrices);
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeSkeleton(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		return s;
	}
}
