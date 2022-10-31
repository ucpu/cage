#include "main.h"

#include <cage-core/files.h>
#include <cage-core/concurrent.h>
#include <cage-core/serialization.h>
#include <cage-core/assetHeader.h>
#include <cage-core/assetContext.h>
#include <cage-core/assetManager.h>
#include <cage-core/assetOnDemand.h>

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
	CAGE_TEST((!cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(13)));
	while (man->processing())
		threadYield();
	CAGE_TEST((cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(13)));
	CAGE_TEST(!(cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(42)));
	cache->process();
	while (man->processing())
		threadYield();
	CAGE_TEST((cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(42)));
	cache->process();
	while (man->processing())
		threadYield();
	CAGE_TEST(!(cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(13)));
	CAGE_TEST((cache->get<AssetSchemeIndexRaw, PointerRange<const char>>(42)));
	cache.clear();
	while (man->processing())
		threadYield();
	man->unloadWait();
}
