#include "main.h"

#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>

namespace
{
	AssetScheme genAssetSchemeVoid()
	{
		AssetScheme s;
		s.typeHash = detail::typeHash<void>();
		return s;
	}
}

void testAssetOnDemand()
{
	CAGE_TESTCASE("asset on demand");

	Holder<AssetManager> man = newAssetManager({});
	man->defineScheme<0, void>(genAssetSchemeVoid());
	Holder<AssetOnDemand> cache = newAssetOnDemand(+man);
	cache->get<0, void>(13);
	cache->process();
	cache->get<0, void>(42);
	cache->process();
}
