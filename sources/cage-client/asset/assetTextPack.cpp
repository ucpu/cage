#include <cage-core/core.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/assets.h>
#include <cage-client/utility/textPack.h>

namespace cage
{
	namespace
	{
		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			textPackClass *texts = nullptr;
			if (!context->assetHolder)
				context->assetHolder = newTextPack().transfev();
			texts = static_cast<textPackClass*> (context->assetHolder.get());
			texts->clear();
			context->returnData = texts;

			pointer ptr = context->originalData;
			uint32 cnt = *(uint32*)context->originalData;
			ptr += 4;
			for (uint32 i = 0; i < cnt; i++)
			{
				uint32 name = *ptr.asUint32++;
				uint32 len = *ptr.asUint32++;
				texts->set(name, string(ptr.asChar, len));
				ptr += len;
			}
			CAGE_ASSERT_RUNTIME(ptr == pointer(context->originalData) + (uintPtr)context->originalSize);
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeTextPackage(const uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = nullptr;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		return s;
	}
}