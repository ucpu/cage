#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/textPack.h>
#include <cage-core/collider.h>
#include <cage-core/assetStructs.h>
#include <cage-core/assetManager.h>

namespace cage
{
	/*
	struct CAGE_CORE_API TextPackHeader
	{
		// follows:
		// count, uint32, number of texts
		// name, uint32
		// length, uint32
		// array of chars
		// name, uint32
		// ...
	};

	struct CAGE_CORE_API ColliderHeader
	{
		// follows:
		// serialized collider data (possibly compressed)
	};
	*/

	namespace
	{
		void defaultDecompress(const AssetContext *context)
		{
			if (context->compressedData().size() == 0)
				return;
			PointerRange<char> orig = context->originalData();
			detail::decompress(context->compressedData(), orig);
			CAGE_ASSERT(orig.size() == context->originalData().size());
		}
	}

	AssetContext::AssetContext(uint32 realName) : realName(realName)
	{
		textName = stringizer() + "<" + realName + ">";
	}

	AssetScheme::AssetScheme()
	{
		decompress.bind<&defaultDecompress>();
	}

	namespace
	{
		void processAssetPackLoad(const AssetContext *context)
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
		void processRawLoad(const AssetContext *context)
		{
			Holder<MemoryBuffer> mem = detail::systemArena().createHolder<MemoryBuffer>(templates::move(context->originalData()));
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
		void processTextPackLoad(const AssetContext *context)
		{
			Holder<TextPack> texts = newTextPack();
			Deserializer des(context->originalData());
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
		void processCollisionMeshLoad(const AssetContext *context)
		{
			Holder<Collider> col = newCollider();
			col->deserialize(context->originalData());
			col->rebuild();
			context->assetHolder = templates::move(col).cast<void>();
		}
	}

	AssetScheme genAssetSchemeCollider()
	{
		AssetScheme s;
		s.load.bind<&processCollisionMeshLoad>();
		return s;
	}

	namespace detail
	{
		template<> CAGE_API_EXPORT char assetClassId<AssetPack>;
		template<> CAGE_API_EXPORT char assetClassId<MemoryBuffer>;
		template<> CAGE_API_EXPORT char assetClassId<TextPack>;
		template<> CAGE_API_EXPORT char assetClassId<Collider>;
	}
}
