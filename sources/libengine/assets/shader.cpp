#include <cage-core/assetContext.h>
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
			Holder<MultiShader> shr = newMultiShader();

			Deserializer des(context->originalData);
			MultiShaderHeader header;
			des >> header;

			// todo
			CAGE_ASSERT(des.available() == 0);

			context->assetHolder = std::move(shr).cast<void>();
		}
	}

	AssetsScheme genAssetSchemeShader(GraphicsDevice *device, uint32 threadIndex)
	{
		AssetsScheme s;
		s.device = device;
		s.threadIndex = threadIndex;
		s.load.bind<processLoad>();
		s.typeHash = detail::typeHash<MultiShader>();
		return s;
	}
}
