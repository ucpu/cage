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
			shaderProgram *shr = nullptr;
			if (context->assetHolder)
			{
				shr = static_cast<shaderProgram*>(context->assetHolder.get());
				shr->bind();
			}
			else
			{
				context->assetHolder = newShaderProgram().cast<void>();
				shr = static_cast<shaderProgram*>(context->assetHolder.get());
				shr->setDebugName(context->textName);
			}
			context->returnData = shr;

			Deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
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
			CAGE_ASSERT(des.available() == 0);
		}
	}

	AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex, windowHandle *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
