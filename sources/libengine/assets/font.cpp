#include <cage-core/assetContext.h>
#include <cage-core/typeIndex.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/font.h>

namespace cage
{
	namespace
	{
		void processLoad(AssetContext *context)
		{
			Holder<Font> font = newFont(context->textId);
			font->importBuffer(context->originalData);
			context->assetHolder = std::move(font).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeFont()
	{
		AssetsScheme s;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<Font>();
		return s;
	}
}
