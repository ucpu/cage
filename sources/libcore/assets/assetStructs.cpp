#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/textPack.h>
#include <cage-core/collisionMesh.h>
#include <cage-core/assetStructs.h>
#include <cage-core/assetManager.h>

namespace cage
{
	namespace
	{
		void defaultDecompress(const AssetContext *context, void *)
		{
			if (context->compressedData().size() == 0)
				return;
			uintPtr res = detail::decompress(context->compressedData().data(), context->compressedData().size(), context->originalData().data(), context->originalData().size());
			CAGE_ASSERT(res == context->originalData().size(), res, context->originalData().size());
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
		void processAssetPackLoad(const AssetContext *context, void *)
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
		void processRawLoad(const AssetContext *context, void *)
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
		void processTextPackLoad(const AssetContext *context, void *)
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
		void processCollisionMeshLoad(const AssetContext *context, void *)
		{
			Holder<CollisionMesh> col = newCollisionMesh();
			col->deserialize(context->originalData());
			col->rebuild();
			context->assetHolder = templates::move(col).cast<void>();
		}
	}

	AssetScheme genAssetSchemeCollisionMesh()
	{
		AssetScheme s;
		s.load.bind<&processCollisionMeshLoad>();
		return s;
	}

	namespace detail
	{
		template<> GCHL_API_EXPORT char assetClassId<AssetPack>;
		template<> GCHL_API_EXPORT char assetClassId<MemoryBuffer>;
		template<> GCHL_API_EXPORT char assetClassId<TextPack>;
		template<> GCHL_API_EXPORT char assetClassId<CollisionMesh>;
	}
}
