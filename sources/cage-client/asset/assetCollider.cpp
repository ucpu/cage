#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/utility/collider.h>
#include <cage-core/utility/memoryBuffer.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/assets.h>

namespace cage
{
	namespace
	{
		void processDecompress(const assetContextStruct *context, void *schemePointer)
		{
			uintPtr res = detail::decompress((char*)context->compressedData, context->compressedSize, (char*)context->originalData, context->originalSize);
			CAGE_ASSERT_RUNTIME(res == context->originalSize, res, context->originalSize);
		}

		void processLoad(const assetContextStruct *context, void *schemePointer)
		{
			if (!context->assetHolder)
				context->assetHolder = newCollider().transfev();
			colliderClass *col = static_cast<colliderClass*>(context->assetHolder.get());
			context->returnData = col;

			memoryBuffer buff(context->originalSize);
			detail::memcpy(buff.data(), context->originalData, context->originalSize);
			col->deserialize(buff);
			col->rebuild();
		}

		void processDone(const assetContextStruct *context, void *schemePointer)
		{
			context->assetHolder.clear();
			context->returnData = nullptr;
		}
	}

	assetSchemeStruct genAssetSchemeCollider(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.load.bind<&processLoad>();
		s.done.bind<&processDone>();
		s.decompress.bind<&processDecompress>();
		return s;
	}
}
