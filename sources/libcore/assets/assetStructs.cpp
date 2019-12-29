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
	AssetContext::AssetContext() : compressedSize(0), originalSize(0), assetPointer(nullptr), returnData(nullptr), compressedData(nullptr), originalData(nullptr), realName(0), aliasName(0), assetFlags(0) {}
	AssetScheme::AssetScheme() : schemePointer(nullptr), threadIndex(0) {}

	AssetScheme genAssetSchemePack(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = nullptr;
		return s;
	}

	namespace
	{
		Holder<MemoryBuffer> newMemoryBuffer()
		{
			return detail::systemArena().createHolder<MemoryBuffer>();
		}

		void processRawLoad(const AssetContext *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newMemoryBuffer().cast<void>();
			MemoryBuffer *mem = static_cast<MemoryBuffer*>(context->assetHolder.get());
			context->returnData = mem;
			mem->allocate(numeric_cast<uintPtr>(context->originalSize));
			detail::memcpy(mem->data(), context->originalData, mem->size());
		}
	}

	AssetScheme genAssetSchemeRaw(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processRawLoad>();
		return s;
	}

	namespace
	{
		void processTextLoad(const AssetContext *context, void *schemePointer)
		{
			TextPack *texts = nullptr;
			if (!context->assetHolder)
				context->assetHolder = newTextPack().cast<void>();
			texts = static_cast<TextPack*>(context->assetHolder.get());
			texts->clear();
			context->returnData = texts;

			Deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
			uint32 cnt;
			des >> cnt;
			for (uint32 i = 0; i < cnt; i++)
			{
				uint32 name;
				string val;
				des >> name >> val;
				texts->set(name, val);
			}
		}
	}

	AssetScheme genAssetSchemeTextPackage(const uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.schemePointer = nullptr;
		s.load.bind<&processTextLoad>();
		return s;
	}

	namespace
	{
		void processColliderLoad(const AssetContext *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newCollisionMesh().cast<void>();
			CollisionMesh *col = static_cast<CollisionMesh*>(context->assetHolder.get());
			context->returnData = col;

			MemoryBuffer buff(numeric_cast<uintPtr>(context->originalSize));
			detail::memcpy(buff.data(), context->originalData, numeric_cast<uintPtr>(context->originalSize));
			col->deserialize(buff);
			col->rebuild();
		}
	}

	AssetScheme genAssetSchemeCollisionMesh(uint32 threadIndex)
	{
		AssetScheme s;
		s.threadIndex = threadIndex;
		s.load.bind<&processColliderLoad>();
		return s;
	}
}
