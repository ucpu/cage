#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryCompression.h>
#include <cage-core/serialization.h>
#include <cage-core/textPack.h>
#include <cage-core/collider.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>

namespace cage
{
	namespace
	{
		void defaultDecompress(AssetContext *context)
		{
			if (context->compressedData.size() == 0)
				return;
			PointerRange<char> orig = context->originalData;
			decompress(context->compressedData, orig);
			CAGE_ASSERT(orig.size() == context->originalData.size());
		}
	}

	AssetContext::AssetContext(const detail::StringBase<64> &textName, MemoryBuffer &compressedData, MemoryBuffer &originalData, Holder<void> &assetHolder, uint32 realName) : textName(textName), compressedData(compressedData), originalData(originalData), assetHolder(assetHolder), realName(realName)
	{}

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
		s.typeIndex = detail::typeIndex<AssetPack>();
		return s;
	}

	namespace
	{
		void processRawLoad(AssetContext *context)
		{
			Holder<MemoryBuffer> mem = systemMemory().createHolder<MemoryBuffer>(std::move(context->originalData));
			context->assetHolder = std::move(mem).cast<void>();
		}
	}

	AssetScheme genAssetSchemeRaw()
	{
		AssetScheme s;
		s.load.bind<&processRawLoad>();
		s.typeIndex = detail::typeIndex<MemoryBuffer>();
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
				string val;
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
		s.typeIndex = detail::typeIndex<TextPack>();
		return s;
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
		s.typeIndex = detail::typeIndex<Collider>();
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
		s.typeIndex = detail::typeIndex<SkeletalAnimation>();
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
		s.typeIndex = detail::typeIndex<SkeletonRig>();
		return s;
	}
}
