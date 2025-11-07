#include <vector>

#include <cage-core/assetContext.h>
#include <cage-core/containerSerialization.h>
#include <cage-core/serialization.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/assetsStructs.h>
#include <cage-engine/shader.h>
#include <cage-engine/spirv.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<MultiShader> shr = newMultiShader(context->textId);

			Deserializer des(context->originalData);
			MultiShaderHeader header;
			des >> header;

			std::vector<detail::StringBase<20>> keywords;
			des >> keywords;
			shr->setKeywords(keywords);

			for (uint32 i = 0; i < header.variantsCount; i++)
			{
				uint32 id = 0, size = 0;
				des >> id >> size;
				Holder<Spirv> s = newSpirv();
				s->importBuffer(des.read(size));
				shr->addVariant((GraphicsDevice *)context->device, id, +s);
			}

			CAGE_ASSERT(des.available() == 0);

			shr->customDataCount = header.customDataCount;

			context->assetHolder = std::move(shr).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeShader(GraphicsDevice *device)
	{
		AssetsScheme s;
		s.device = device;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<MultiShader>();
		return s;
	}
}
