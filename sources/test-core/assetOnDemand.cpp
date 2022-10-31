#include "main.h"

#include <cage-core/files.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>

namespace
{
	const String AssetsPath = pathJoin(pathWorkingDir(), "testdir/assetManager/assets");

	void processAssetDummyLoad(AssetContext *context)
	{
		// nothing
	}

	AssetScheme genAssetSchemeVoid()
	{
		AssetScheme s;
		s.load.bind<&processAssetDummyLoad>();
		s.typeHash = detail::typeHash<void>();
		return s;
	}
}

void testAssetOnDemand()
{
	CAGE_TESTCASE("asset on demand");

	pathCreateDirectories(AssetsPath);
	AssetManagerCreateConfig cfg;
	cfg.assetsFolderName = AssetsPath;
	Holder<AssetManager> man = newAssetManager(cfg);
	man->defineScheme<0, void>(genAssetSchemeVoid());
	Holder<AssetOnDemand> cache = newAssetOnDemand(+man);
	cache->get<0, void>(13);
	cache->process();
	cache->get<0, void>(42);
	cache->process();
}
