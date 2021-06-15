#include <cage-core/assetContext.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/typeIndex.h>

#include <cage-engine/graphics.h>
#include <cage-engine/assetStructs.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<ShaderProgram> shr = newShaderProgram();
			shr->setDebugName(context->textName);

			Deserializer des(context->originalData);
			uint32 count;
			des >> count;
			for (uint32 i = 0; i < count; i++)
			{
				uint32 type, len;
				des >> type >> len;
				PointerRange<const char> pos = des.read(len);
				shr->source(type, pos);
			}
			shr->relink();
			CAGE_ASSERT(des.available() == 0);

			context->assetHolder = std::move(shr).cast<void>();
		}
	}

	AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.typeIndex = detail::typeIndex<ShaderProgram>();
		return s;
	}
}
