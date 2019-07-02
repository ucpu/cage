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
			shaderClass *shr = nullptr;
			if (context->assetHolder)
			{
				shr = static_cast<shaderClass*>(context->assetHolder.get());
				shr->bind();
			}
			else
			{
				context->assetHolder = newShader().cast<void>();
				shr = static_cast<shaderClass*>(context->assetHolder.get());
				shr->setDebugName(context->textName);
			}
			context->returnData = shr;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			uint32 count;
			des >> count;
			for (uint32 i = 0; i < count; i++)
			{
				uint32 type, len;
				des >> type >> len;
				const char *pos = (const char *)des.advance(len);
				shr->source(type, pos, len);
			}
			shr->relink();
			CAGE_ASSERT_RUNTIME(des.available() == 0);
		}
	}

	assetScheme genAssetSchemeShader(uint32 threadIndex, windowClass *memoryContext)
	{
		assetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
