#include <cage-core/assetStructs.h>
#include <cage-core/serialization.h>

#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(const AssetContext *context, void *schemePointer)
		{
			Holder<ShaderProgram> shr = newShaderProgram();
			shr->setDebugName(context->textName);

			Deserializer des(context->originalData());
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

			context->assetHolder = templates::move(shr).cast<void>();
		}
	}

	AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex, Window *memoryContext)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = memoryContext;
		s.load.bind<&processLoad>();
		return s;
	}
}
