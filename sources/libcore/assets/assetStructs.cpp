#include <vector>

#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>
#include <cage-core/collider.h>
#include <cage-core/math.h>
#include <cage-core/memoryCompression.h>
#include <cage-core/serialization.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/string.h>
#include <cage-core/textPack.h>

namespace cage
{
	namespace
	{
		void defaultDecompress(AssetContext *context)
		{
			if (context->compressedData.size() == 0)
				return;
			decompress(context->compressedData, context->originalData);
		}
	}

	AssetContext::AssetContext(const detail::StringBase<64> &textName, PointerRange<char> compressedData, PointerRange<char> originalData, Holder<void> &assetHolder, uint32 realName) : textName(textName), compressedData(compressedData), originalData(originalData), assetHolder(assetHolder), realName(realName) {}

	AssetScheme::AssetScheme()
	{
		decompress.bind<&defaultDecompress>();
	}

	namespace
	{
		void processAssetPackLoad(AssetContext *context)
		{
			static AssetPack pack;
			context->assetHolder = Holder<AssetPack>(&pack, nullptr).cast<void>();
		}
	}

	AssetScheme genAssetSchemePack()
	{
		AssetScheme s;
		s.load.bind<&processAssetPackLoad>();
		s.typeHash = detail::typeHash<AssetPack>();
		return s;
	}

	namespace
	{
		void processRawLoad(AssetContext *context)
		{
			Holder<PointerRange<char>> h = systemMemory().createBuffer(context->originalData.size());
			CAGE_ASSERT(h.size() > 0);
			detail::memcpy(h.data(), context->originalData.data(), h.size());
			context->assetHolder = std::move(h).cast<void>();
		}
	}

	AssetScheme genAssetSchemeRaw()
	{
		AssetScheme s;
		s.load.bind<&processRawLoad>();
		s.typeHash = detail::typeHash<PointerRange<const char>>();
		return s;
	}

	namespace
	{
		void processTextPackLoad(AssetContext *context)
		{
			Holder<TextPack> texts = newTextPack();
			Deserializer des(context->originalData);
			uint32 cnt;
			des >> cnt;
			for (uint32 i = 0; i < cnt; i++)
			{
				uint32 name;
				String val;
				des >> name >> val;
				texts->set(name, val);
			}
			context->assetHolder = std::move(texts).cast<void>();
		}
	}

	AssetScheme genAssetSchemeTextPack()
	{
		AssetScheme s;
		s.load.bind<&processTextPackLoad>();
		s.typeHash = detail::typeHash<TextPack>();
		return s;
	}

	String loadFormattedString(AssetManager *assets, uint32 asset, uint32 text, String params)
	{
		if (asset == 0 || text == 0)
			return params;
		auto a = assets->tryGet<AssetSchemeIndexTextPack, TextPack>(asset);
		if (a)
		{
			std::vector<String> ps;
			while (!params.empty())
				ps.push_back(split(params, "|"));
			return a->format(text, ps);
		}
		else
			return "";
	}

	namespace
	{
		void processColliderLoad(AssetContext *context)
		{
			Holder<Collider> col = newCollider();
			col->importBuffer(context->originalData);
			col->rebuild();
			context->assetHolder = std::move(col).cast<void>();
		}
	}

	AssetScheme genAssetSchemeCollider()
	{
		AssetScheme s;
		s.load.bind<&processColliderLoad>();
		s.typeHash = detail::typeHash<Collider>();
		return s;
	}

	namespace
	{
		void processSkeletalAnimationLoad(AssetContext *context)
		{
			Holder<SkeletalAnimation> ani = newSkeletalAnimation();
			ani->importBuffer(context->originalData);
			context->assetHolder = std::move(ani).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSkeletalAnimation()
	{
		AssetScheme s;
		s.load.bind<&processSkeletalAnimationLoad>();
		s.typeHash = detail::typeHash<SkeletalAnimation>();
		return s;
	}

	namespace
	{
		void processSkeletonRigLoad(AssetContext *context)
		{
			Holder<SkeletonRig> skl = newSkeletonRig();
			skl->importBuffer(context->originalData);
			context->assetHolder = std::move(skl).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSkeletonRig()
	{
		AssetScheme s;
		s.load.bind<&processSkeletonRigLoad>();
		s.typeHash = detail::typeHash<SkeletonRig>();
		return s;
	}
}
