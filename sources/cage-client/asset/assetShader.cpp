#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/assets.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			windowClass *gm = (windowClass *)schemePointer;

			shaderClass *shr = nullptr;
			if (context->assetHolder)
			{
				shr = static_cast<shaderClass*> (context->assetHolder.get());
				shr->bind();
			}
			else
			{
				context->assetHolder = newShader(gm).transfev();
				shr = static_cast<shaderClass*> (context->assetHolder.get());
			}
			context->returnData = shr;

			uint32 count = *(uint32*)context->originalData;
			char *pos = (char*)context->originalData + sizeof(count);
			for (uint32 i = 0; i < count; i++)
			{
				uint32 type = *(uint32*)pos;
				pos += sizeof(type);
				uint32 len = *(uint32*)pos;
				pos += sizeof(len);
				shr->source(type, pos, len);
				pos += len;
			}
			shr->relink();
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeShader(uint32 threadIndex, windowClass *memoryContext)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind <&processLoad>();
		s.done.bind <&processDone>();
		return s;
	}
}