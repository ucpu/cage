#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#include <cage-core/utility/pointer.h>
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
				skl = static_cast<skeletonClass*> (context->assetHolder.get());
			}
			else
			{
				context->assetHolder = newSkeleton().transfev();
				skl = static_cast<skeletonClass*> (context->assetHolder.get());
			}
			context->returnData = skl;

			skeletonHeaderStruct *data = (skeletonHeaderStruct*)context->originalData;
			pointer ptr = pointer(context->originalData) + sizeof(skeletonHeaderStruct);
			const uint16 *boneParents = (uint16*)ptr.asVoid;
			ptr += sizeof(uint16) * data->bonesCount;
			const mat4 *baseMatrices = (mat4*)ptr.asVoid;
			ptr += sizeof(mat4) * data->bonesCount;
			const mat4 *invRestMatrices = (mat4*)ptr.asVoid;
			ptr += sizeof(mat4) * data->bonesCount;
			const uint32 *namedBoneNames = (uint32*)ptr.asVoid;
			ptr += sizeof(uint32) * data->namesCount;
			const uint16 *namedBoneIndexes = (uint16*)ptr.asVoid;
			ptr += sizeof(mat4) * data->namesCount;
			skl->allocate(data->bonesCount, boneParents, baseMatrices, invRestMatrices, data->namesCount, namedBoneNames, namedBoneIndexes);
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
		s.load.bind <&processLoad>();
		s.done.bind <&processDone>();
		return s;
	}
}