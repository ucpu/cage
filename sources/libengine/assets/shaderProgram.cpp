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
			ShaderProgramHeader header;
			des >> header;
			shr->customDataCount = header.customDataCount;
			{
				std::vector<detail::StringBase<20>> keywords;
				keywords.reserve(header.keywordsCount);
				for (uint32 i = 0; i < header.keywordsCount; i++)
				{
					detail::StringBase<20> s;
					des >> s;
					keywords.push_back(s);
				}
				shr->setKeywords(keywords);
			}
			for (uint32 i = 0; i < header.stagesCount; i++)
			{
				uint32 type, len;
				des >> type >> len;
				PointerRange<const char> pos = des.read(len);
				shr->setSource(type, pos);
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
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<MultiShaderProgram>();
		return s;
	}
}
