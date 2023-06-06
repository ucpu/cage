#include "main.h"

#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/serialization.h>

namespace
{
	const String AssetsPath = pathJoin(pathWorkingDir(), "testdir/assetManager/assets");

	void makeAssetRaw(uint32 name, PointerRange<const char> contents)
	{
		AssetHeader hdr(Stringizer() + name, AssetSchemeIndexRaw);
		hdr.originalSize = contents.size();
		Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
		f->write(bufferView(hdr));
		f->write(contents);
		f->close();
	}
}

void testAssetOnDemand()
{
	CAGE_TESTCASE("assets on demand");

	pathCreateDirectories(AssetsPath);
	AssetManagerCreateConfig cfg;
	cfg.assetsFolderName = AssetsPath;
	Holder<AssetManager> man = newAssetManager(cfg);
	man->defineScheme<AssetSchemeIndexRaw, PointerRange<const char>>(genAssetSchemeRaw());
	Holder<AssetOnDemand> cache = newAssetOnDemand(+man);
	makeAssetRaw(13, "hello world");
	makeAssetRaw(42, "the ultimate question of life, the universe, and everything");
	CAGE_TEST((!cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(13))); // 13 not yet loaded, but requested now
	while (man->processing())
		threadYield();
	CAGE_TEST((cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(13))); // 13 loaded, refreshed
	CAGE_TEST(!(cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(42))); // 42 not yet loaded, requested now
	for (uint32 i = 0; i < 12; i++)
		cache->process(); // few initial processing does not release any assets
	while (man->processing())
		threadYield();
	// 13 not refreshed
	CAGE_TEST((cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(42))); // 42 loaded, refreshed
	for (uint32 i = 0; i < 12; i++)
		cache->process(); // several more processing releases 13 but keeps 42
	while (man->processing())
		threadYield();
	CAGE_TEST(!(cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(13))); // 13 not loaded
	CAGE_TEST((cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(42))); // 42 loaded
	cache.clear();
	while (man->processing())
		threadYield();
	man->unloadWait();
}
