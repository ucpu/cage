#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/assets.h>
#include <cage-core/utility/textPack.h>
#include <cage-core/utility/serialization.h>

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

			deserializer des(context->originalData, context->originalSize);
			uint32 cnt;
			des >> cnt;
			for (uint32 i = 0; i < cnt; i++)
			{
				uint32 name;
				string val;
				des >> name >> val;
				texts->set(name, val);
			}
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
