#include <cage-core/math.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/textPack.h>
#include <cage-core/collider.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>

namespace cage
{
	/*
	struct TextPackHeader
	{
		// follows:
		// count, uint32, number of texts
		// name, uint32
		// length, uint32
		// array of chars
		// name, uint32
		// ...
	};

	struct ColliderHeader
	{
		// follows:
		// serialized collider data (possibly compressed)
	};

	struct SkeletalAnimationHeader
	{
		uint64 duration; // microseconds
		uint32 skeletonBonesCount;
		uint32 animationBonesCount;

		// follows:
		// array of indices, each uint16
		// array of position frames counts, each uint16
		// array of rotation frames counts, each uint16
		// array of scale frames counts, each uint16
		// array of bones, each:
		//   array of position times, each float
		//   array of position values, each vec3
		//   array of rotation times, each float
		//   array of rotation values, each quat
		//   array of scale times, each float
		//   array of scale values, each vec3
	};

	struct SkeletonRigHeader
	{
		mat4 globalInverse;
		uint32 bonesCount;

		// follows:
		// array of parent bone indices, each uint16, (uint16)-1 for the root
		// array of base transformation matrices, each mat4
		// array of inverted rest matrices, each mat4
	};
	*/

	namespace
	{
		void defaultDecompress(AssetContext *context)
		{
			if (context->compressedData.size() == 0)
				return;
			PointerRange<char> orig = context->originalData;
			detail::decompress(context->compressedData, orig);
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
			Holder<AssetPack> h = Holder<AssetPack>(&pack, nullptr, {});
			context->assetHolder = templates::move(h).cast<void>();
		}
	}

	AssetScheme genAssetSchemePack()
	{
		AssetScheme s;
		s.load.bind<&processAssetPackLoad>();
		return s;
	}

	namespace
	{
		void processRawLoad(AssetContext *context)
		{
			Holder<MemoryBuffer> mem = detail::systemArena().createHolder<MemoryBuffer>(templates::move(context->originalData));
			context->assetHolder = templates::move(mem).cast<void>();
		}
	}

	AssetScheme genAssetSchemeRaw()
	{
		AssetScheme s;
		s.load.bind<&processRawLoad>();
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
			context->assetHolder = templates::move(texts).cast<void>();
		}
	}

	AssetScheme genAssetSchemeTextPack()
	{
		AssetScheme s;
		s.load.bind<&processTextPackLoad>();
		return s;
	}

	namespace
	{
		void processColliderLoad(AssetContext *context)
		{
			Holder<Collider> col = newCollider();
			col->deserialize(context->originalData);
			col->rebuild();
			context->assetHolder = templates::move(col).cast<void>();
		}
	}

	AssetScheme genAssetSchemeCollider()
	{
		AssetScheme s;
		s.load.bind<&processColliderLoad>();
		return s;
	}

	namespace
	{
		void processSkeletalAnimationLoad(AssetContext *context)
		{
			Holder<SkeletalAnimation> ani = newSkeletalAnimation();
			Deserializer des(context->originalData);
			uint32 skeletonBonesCount;
			uint32 animationBonesCount;
			des >> ani->duration;
			des >> skeletonBonesCount;
			des >> animationBonesCount;
			ani->deserialize(animationBonesCount, des.advance(des.available()));
			context->assetHolder = templates::move(ani).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSkeletalAnimation()
	{
		AssetScheme s;
		s.load.bind<&processSkeletalAnimationLoad>();
		return s;
	}

	namespace
	{
		void processSkeletonRigLoad(AssetContext *context)
		{
			Holder<SkeletonRig> skl = newSkeletonRig();
			Deserializer des(context->originalData);
			mat4 globalInverse;
			uint32 bonesCount;
			des >> globalInverse;
			des >> bonesCount;
			skl->deserialize(globalInverse, bonesCount, des.advance(des.available()));
			context->assetHolder = templates::move(skl).cast<void>();
		}
	}

	AssetScheme genAssetSchemeSkeletonRig()
	{
		AssetScheme s;
		s.load.bind<&processSkeletonRigLoad>();
		return s;
	}

	namespace detail
	{
		template<> CAGE_API_EXPORT char assetClassId<AssetPack>;
		template<> CAGE_API_EXPORT char assetClassId<MemoryBuffer>;
		template<> CAGE_API_EXPORT char assetClassId<TextPack>;
		template<> CAGE_API_EXPORT char assetClassId<Collider>;
		template<> CAGE_API_EXPORT char assetClassId<SkeletalAnimation>;
		template<> CAGE_API_EXPORT char assetClassId<SkeletonRig>;
	}
}
