#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/textPack.h>
#include <cage-core/collider.h>
#include <cage-core/assetStructs.h>
#include <cage-core/assetManager.h>

namespace cage
{
	assetContextStruct::assetContextStruct() : compressedSize(0), originalSize(0), assetPointer(nullptr), returnData(nullptr), compressedData(nullptr), originalData(nullptr), realName(0), internationalizedName(0), assetFlags(0) {}
	assetSchemeStruct::assetSchemeStruct() : schemePointer(nullptr), threadIndex(0) {}

	assetSchemeStruct genAssetSchemePack(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = nullptr;
		return s;
	}

	namespace
	{
		holder<memoryBuffer> newMemoryBuffer()
		{
			return detail::systemArena().createHolder<memoryBuffer>();
		}

		void processRawLoad(const assetContextStruct *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newMemoryBuffer().cast<void>();
			memoryBuffer *mem = static_cast<memoryBuffer*>(context->assetHolder.get());
			context->returnData = mem;
			mem->allocate(numeric_cast<uintPtr>(context->originalSize));
			detail::memcpy(mem->data(), context->originalData, mem->size());
		}
	}

	assetSchemeStruct genAssetSchemeRaw(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processRawLoad>();
		return s;
	}

	namespace
	{
		void processTextLoad(const assetContextStruct *context, void *schemePointer)
		{
			textPackClass *texts = nullptr;
			if (!context->assetHolder)
				context->assetHolder = newTextPack().cast<void>();
			texts = static_cast<textPackClass*>(context->assetHolder.get());
			texts->clear();
			context->returnData = texts;

			deserializer des(context->originalData, numeric_cast<uintPtr>(context->originalSize));
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

	assetSchemeStruct genAssetSchemeTextPackage(const uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = nullptr;
		s.load.bind<&processTextLoad>();
		return s;
	}

	namespace
	{
		void processColliderLoad(const assetContextStruct *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newCollider().cast<void>();
			colliderClass *col = static_cast<colliderClass*>(context->assetHolder.get());
			context->returnData = col;

			memoryBuffer buff(numeric_cast<uintPtr>(context->originalSize));
			detail::memcpy(buff.data(), context->originalData, numeric_cast<uintPtr>(context->originalSize));
			col->deserialize(buff);
			col->rebuild();
		}
	}

	assetSchemeStruct genAssetSchemeCollider(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processColliderLoad>();
		return s;
	}
}
