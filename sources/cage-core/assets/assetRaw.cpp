#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/assets.h>
#include <cage-core/utility/memoryBuffer.h>

namespace cage
{
	namespace
	{
		void processDecompress(const assetContextStruct *context, void *schemePointer)
		{
			if (context->compressedData == context->originalData)
				return;
			uintPtr res = detail::decompress((char*)context->compressedData, numeric_cast<uintPtr>(context->compressedSize), (char*)context->originalData, numeric_cast<uintPtr>(context->originalSize));
			CAGE_ASSERT_RUNTIME(res == context->originalSize, res, context->originalSize);
		}

		holder<memoryBuffer> newMemoryBuffer()
		{
			return detail::systemArena().createHolder<memoryBuffer>();
		}

		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newMemoryBuffer().transfev();
			memoryBuffer *mem = static_cast<memoryBuffer*>(context->assetHolder.get());
			context->returnData = mem;
			mem->reallocate(numeric_cast<uintPtr>(context->originalSize));
			detail::memcpy(mem->data(), context->originalData, mem->size());
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeRaw(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		s.decompress.bind<&processDecompress>();
		return s;
	}
}
