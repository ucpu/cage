#include <cage-core/core.h>
#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/assets.h>

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