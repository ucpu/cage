#include <vector>

#include <cage-core/assetContext.h>
#include <cage-core/assetsManager.h>
#include <cage-core/collider.h>
#include <cage-core/serialization.h>
#include <cage-core/skeletalAnimation.h>

namespace cage
{
	namespace
	{
		void processAssetPackLoad(AssetContext *context)
		{
			static AssetPack pack;
			context->assetHolder = Holder<AssetPack>(&pack, nullptr).cast<void>();
		}
	}

	AssetsScheme genAssetSchemePack()
	{
		AssetsScheme s;
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

	AssetsScheme genAssetSchemeRaw()
	{
		AssetsScheme s;
		s.load.bind<&processRawLoad>();
		s.typeHash = detail::typeHash<PointerRange<const char>>();
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

	AssetsScheme genAssetSchemeCollider()
	{
		AssetsScheme s;
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

	AssetsScheme genAssetSchemeSkeletalAnimation()
	{
		AssetsScheme s;
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

	AssetsScheme genAssetSchemeSkeletonRig()
	{
		AssetsScheme s;
		s.load.bind<&processSkeletonRigLoad>();
		s.typeHash = detail::typeHash<SkeletonRig>();
		return s;
	}
}
