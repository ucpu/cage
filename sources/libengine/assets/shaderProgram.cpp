#include <vector>

#include <cage-core/assetContext.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/shaderProgram.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<MultiShaderProgram> shr = newMultiShaderProgram();
			shr->setDebugName(context->textId);

			Deserializer des(context->originalData);
			{
				std::vector<detail::StringBase<20>> keywords;
				uint32 count;
				des >> count;
				for (uint32 i = 0; i < count; i++)
				{
					detail::StringBase<20> s;
					des >> s;
					keywords.push_back(s);
				}
				shr->setKeywords(keywords);
			}
			{
				uint32 count;
				des >> count;
				for (uint32 i = 0; i < count; i++)
				{
					uint32 type, len;
					des >> type >> len;
					PointerRange<const char> pos = des.read(len);
					shr->setSource(type, pos);
				}
			}
			CAGE_ASSERT(des.available() == 0);

			shr->compile();

			context->assetHolder = std::move(shr).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeShaderProgram(uint32 threadIndex)
	{
		AssetsScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.typeHash = detail::typeHash<MultiShaderProgram>();
		return s;
	}
}
