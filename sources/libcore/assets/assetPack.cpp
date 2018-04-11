#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/assets.h>

namespace cage
{
	assetSchemeStruct genAssetSchemePack(uint32 threadIndex)
	{
		assetSchemeStruct s;
		s.threadIndex = threadIndex;
		s.schemePointer = nullptr;
		return s;
	}
}