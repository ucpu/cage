#include <atomic>

#include "main.h"

#include <cage-core/assetContext.h>
#include <cage-core/assetHeader.h>
#include <cage-core/assetManager.h>
#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/math.h>
#include <cage-core/serialization.h>

namespace
{
	const String AssetsPath = pathJoin(pathWorkingDir(), "testdir/assetManager/assets");

	constexpr uint32 AssetSchemeIndexCounter = 13;

	struct AssetCounter : private Immovable
	{
		static inline std::atomic<sint32> counter = 0;

		AssetCounter()
		{
			CAGE_ASSERT(counter >= 0);
			counter++;
		}

		~AssetCounter()
		{
			counter--;
			CAGE_ASSERT(counter >= 0);
		}
	};

	void processAssetCounterLoad(AssetContext *context)
	{
		if (context->realName >= 5000)
			CAGE_THROW_ERROR(Exception, "intentionally failed asset processing");
		Holder<AssetCounter> h = systemMemory().createHolder<AssetCounter>();
		context->assetHolder = std::move(h).cast<void>();
	}

	AssetScheme genAssetSchemeCounter()
	{
		AssetScheme s;
		s.load.bind<&processAssetCounterLoad>();
		s.typeHash = detail::typeHash<AssetCounter>();
		return s;
	}

	void makeAssetPack(uint32 name, PointerRange<const uint32> deps)
	{
		AssetHeader hdr(Stringizer() + name, AssetSchemeIndexPack);
		hdr.dependenciesCount = numeric_cast<uint16>(deps.size());
		Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
		f->write(bufferView(hdr));
		f->write(bufferCast<const char, const uint32>(deps));
		f->close();
	}

	void makeAssetRaw(uint32 name, PointerRange<const char> contents)
	{
		AssetHeader hdr(Stringizer() + name, AssetSchemeIndexRaw);
		hdr.originalSize = contents.size();
		Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
		f->write(bufferView(hdr));
		f->write(contents);
		f->close();
	}

	void makeAssetCounter(uint32 name, PointerRange<const uint32> deps)
	{
		AssetHeader hdr(Stringizer() + name, AssetSchemeIndexCounter);
		hdr.dependenciesCount = numeric_cast<uint16>(deps.size());
		Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
		f->write(bufferView(hdr));
		f->write(bufferCast(deps));
		f->close();
	}

	void makeAssetCounter(uint32 name)
	{
		makeAssetCounter(name, {});
	}

	Holder<AssetManager> instantiate()
	{
		CAGE_TEST(AssetCounter::counter == 0);
		pathRemove(AssetsPath);
		pathCreateDirectories(AssetsPath);
		AssetManagerCreateConfig cfg;
		cfg.assetsFolderName = AssetsPath;
		Holder<AssetManager> man = newAssetManager(cfg);
		man->defineScheme<AssetSchemeIndexPack, AssetPack>(genAssetSchemePack());
		man->defineScheme<AssetSchemeIndexRaw, PointerRange<const char>>(genAssetSchemeRaw());
		man->defineScheme<AssetSchemeIndexCounter, AssetCounter>(genAssetSchemeCounter());
		return man;
	}

	void waitProcessing(Holder<AssetManager> &man)
	{
		while (man->processing())
			threadYield();
	}

	template<uint32 Length>
	void checkContents(Holder<AssetManager> &man, const uint32 name, const char (&content)[Length])
	{
		Holder<PointerRange<const char>> a = man->get<AssetSchemeIndexRaw, PointerRange<const char>>(10);
		CAGE_TEST(a);
		CAGE_TEST(a->size() == Length - 1);
		CAGE_TEST(detail::memcmp(a->data(), content, Length - 1) == 0);
	}

	void processAssetDummyLoad(AssetContext *context)
	{
		// nothing
	}

	AssetScheme genDummyScheme(uint32 typeHash)
	{
		AssetScheme s;
		s.load.bind<&processAssetDummyLoad>();
		s.typeHash = typeHash;
		return s;
	}
}

void testAssetManager()
{
	CAGE_TESTCASE("asset manager");

	//configSetUint32("cage/assets/logLevel", 2);

	{
		CAGE_TESTCASE("basics");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		makeAssetCounter(20);
		makeAssetCounter(30);
		CAGE_TEST(AssetCounter::counter == 0);
		man->load(10);
		man->load(20);
		man->load(30);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 3);
		{
			Holder<AssetCounter> a = man->get<AssetSchemeIndexCounter, AssetCounter>(10);
			CAGE_TEST(a);
			CAGE_TEST(AssetCounter::counter == 3);
		}
		man->unload(10);
		man->unload(20);
		man->unload(30);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("type validation");
		Holder<AssetManager> man = instantiate();
		man->defineScheme<71, uint8>(genDummyScheme(detail::typeHash<uint8>()));
		man->defineScheme<72, uint16>(genDummyScheme(detail::typeHash<uint16>()));
		man->defineScheme<73, uint32>(genDummyScheme(detail::typeHash<uint32>()));
		man->defineScheme<74, uint64>(genDummyScheme(detail::typeHash<uint64>()));
		{
			auto a = man->get<71, uint8>(42);
			CAGE_TEST(!a);
		}
		{
			auto a = man->get<72, uint16>(42);
			CAGE_TEST(!a);
		}
		{
			auto a = man->get<73, uint32>(42);
			CAGE_TEST(!a);
		}
		{
			auto a = man->get<74, uint64>(42);
			CAGE_TEST(!a);
		}
		CAGE_TEST_ASSERTED((man->get<71, uint64>(42)));
		CAGE_TEST_ASSERTED((man->get<72, uint8>(42)));
		CAGE_TEST_ASSERTED((man->get<73, uint16>(42)));
		CAGE_TEST_ASSERTED((man->get<74, uint32>(42)));
	}

	{
		CAGE_TESTCASE("multiple times added same asset");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		CAGE_TEST(AssetCounter::counter == 0);
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		man->unload(10);
		man->waitTillEmpty();
		CAGE_TEST(AssetCounter::counter == 0);
	}

	{
		CAGE_TESTCASE("dependencies 1");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		makeAssetCounter(20);
		makeAssetCounter(30);
		makeAssetPack(50, PointerRange<const uint32>({ (uint32)10, (uint32)20, (uint32)30 }));
		CAGE_TEST(AssetCounter::counter == 0);
		man->load(50);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 3);
		man->unload(50);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("dependencies 2");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		makeAssetCounter(20);
		makeAssetCounter(30);
		makeAssetCounter(50, PointerRange<const uint32>({ (uint32)10, (uint32)20, (uint32)30 }));
		CAGE_TEST(AssetCounter::counter == 0);
		man->load(50);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 4);
		man->unload(50);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("immediate remove");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		man->load(10);
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("repeated add and remove");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		for (uint32 i = 0; i < 10; i++)
		{
			man->load(10);
			man->unload(10);
			if ((i % 3) == 0)
				waitProcessing(man);
		}
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("asset content");
		Holder<AssetManager> man = instantiate();
		static constexpr const char Content[] = "hello world";
		makeAssetRaw(10, Content);
		man->load(10);
		waitProcessing(man);
		checkContents(man, 10, Content);
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("reload asset");
		Holder<AssetManager> man = instantiate();
		static constexpr const char Content1[] = "hello world";
		static constexpr const char Content2[] = "lorem ipsum dolor sit amet";
		makeAssetRaw(10, Content1);
		man->load(10);
		waitProcessing(man);
		makeAssetRaw(10, Content2);
		checkContents(man, 10, Content1);
		man->reload(10);
		waitProcessing(man);
		checkContents(man, 10, Content2);
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("holding asset after remove");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		{
			Holder<AssetCounter> a = man->get<AssetSchemeIndexCounter, AssetCounter>(10);
			man->unload(10);
			waitProcessing(man);
			CAGE_TEST(AssetCounter::counter == 1);
		}
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("fabricated");
		Holder<AssetManager> man = instantiate();
		{
			Holder<AssetCounter> f = systemMemory().createHolder<AssetCounter>();
			man->loadValue<AssetSchemeIndexCounter, AssetCounter>(10, std::move(f));
		}
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(10)));
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("fabricated overlay");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		AssetCounter *ptr = nullptr;
		{
			Holder<AssetCounter> f = systemMemory().createHolder<AssetCounter>();
			ptr = +f;
			man->loadValue<AssetSchemeIndexCounter, AssetCounter>(10, std::move(f));
		}
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(10)).get() == ptr);
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(10)).get() == ptr);
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("non-existent asset");
		Holder<AssetManager> man = instantiate();
		Holder<AssetCounter> a = man->get<AssetSchemeIndexCounter, AssetCounter>(5);
		CAGE_TEST(!a);
	}

	{
		CAGE_TESTCASE("missing asset file");
		Holder<AssetManager> man = instantiate();
		CAGE_TEST(AssetCounter::counter == 0);
		CAGE_TEST(!pathIsFile(pathJoin(AssetsPath, "10")));
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("getting asset with scheme mismatching type");
		Holder<AssetManager> man = instantiate();
		CAGE_TEST_ASSERTED((man->get<AssetSchemeIndexRaw, AssetCounter>(5)));
	}

	{
		CAGE_TESTCASE("getting asset with wrong type");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 1);
		CAGE_TEST((!man->get<AssetSchemeIndexRaw, PointerRange<const char>>(10)));
		man->unload(10);
		man->waitTillEmpty();
		CAGE_TEST(AssetCounter::counter == 0);
	}

	{
		CAGE_TESTCASE("unknown scheme");
		Holder<AssetManager> man = instantiate();
		{
			static constexpr uint32 name = 10;
			AssetHeader hdr(Stringizer() + name, 42);
			Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
			f->write(bufferView(hdr));
			f->close();
		}
		man->load(10);
		waitProcessing(man);
		CAGE_TEST((!man->get<AssetSchemeIndexCounter, AssetCounter>(10)));
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("accessing missing asset file");
		Holder<AssetManager> man = instantiate();
		man->load(10);
		waitProcessing(man);
		CAGE_TEST(!(man->get<AssetSchemeIndexCounter, AssetCounter>(10)));
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("corrupted header");
		Holder<AssetManager> man = instantiate();
		{
			static constexpr uint32 name = 10;
			AssetHeader hdr(Stringizer() + name, AssetSchemeIndexCounter);
			Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
			const auto view1 = bufferView(hdr);
			const auto view2 = PointerRange<const char>(view1.begin(), view1.begin() + randomRange(uintPtr(0), view1.size() - 1u));
			f->write(view2);
			f->close();
		}
		man->load(10);
		waitProcessing(man);
		CAGE_TEST((!man->get<AssetSchemeIndexCounter, AssetCounter>(10)));
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("fail decompression");
		Holder<AssetManager> man = instantiate();
		{
			static constexpr const char Content[] = "lorem ipsum dolor sit amet";
			static constexpr uint32 name = 10;
			AssetHeader hdr(Stringizer() + name, AssetSchemeIndexRaw);
			hdr.compressedSize = sizeof(Content);
			hdr.originalSize = sizeof(Content) * 2;
			Holder<File> f = writeFile(pathJoin(AssetsPath, Stringizer() + name));
			f->write(bufferView(hdr));
			f->write(Content);
			f->close();
		}
		{
			detail::globalBreakpointOverride(false); // the decompression thread would stop on breakpoint otherwise
			man->load(10);
			waitProcessing(man);
			detail::globalBreakpointOverride(true);
		}
		CAGE_TEST_THROWN((man->get<AssetSchemeIndexRaw, PointerRange<const char>>(10)));
		man->unload(10);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("fail processing");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(5000);
		{
			detail::globalBreakpointOverride(false); // the processing thread would stop on breakpoint otherwise
			man->load(5000);
			waitProcessing(man);
			detail::globalBreakpointOverride(true);
		}
		CAGE_TEST_THROWN((man->get<AssetSchemeIndexCounter, AssetCounter>(5000)));
		man->unload(5000);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("fail processing with dependencies 1");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10);
		makeAssetCounter(20);
		makeAssetCounter(5000, PointerRange<const uint32>({ (uint32)10, (uint32)20 }));
		{
			detail::globalBreakpointOverride(false); // the processing thread would stop on breakpoint otherwise
			man->load(5000);
			waitProcessing(man);
			detail::globalBreakpointOverride(true);
		}
		CAGE_TEST(AssetCounter::counter == 2);
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(10)));
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(20)));
		CAGE_TEST_THROWN((man->get<AssetSchemeIndexCounter, AssetCounter>(5000)));
		man->unload(5000);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("fail processing with dependencies 2");
		Holder<AssetManager> man = instantiate();
		makeAssetCounter(10, PointerRange<const uint32>({ (uint32)20, (uint32)5000 }));
		makeAssetCounter(20);
		makeAssetCounter(5000);
		{
			detail::globalBreakpointOverride(false); // the processing thread would stop on breakpoint otherwise
			man->load(10);
			waitProcessing(man);
			detail::globalBreakpointOverride(true);
		}
		CAGE_TEST(AssetCounter::counter == 2);
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(10)));
		CAGE_TEST((man->get<AssetSchemeIndexCounter, AssetCounter>(20)));
		CAGE_TEST_THROWN((man->get<AssetSchemeIndexCounter, AssetCounter>(5000)));
		man->unload(10);
		waitProcessing(man);
		CAGE_TEST(AssetCounter::counter == 0);
		man->waitTillEmpty();
	}

	{
		CAGE_TESTCASE("initializeAssetHeader");

		{
			AssetHeader ass("abcdefghijklmnopqrstuvwxyz", 42);
			CAGE_TEST(String(ass.cageName) == "cageAss");
			CAGE_TEST(ass.version > 0);
			CAGE_TEST(ass.flags == 0);
			CAGE_TEST(String(ass.textName) == "abcdefghijklmnopqrstuvwxyz");
			CAGE_TEST(ass.compressedSize == 0);
			CAGE_TEST(ass.originalSize == 0);
			CAGE_TEST(ass.scheme == 42);
			CAGE_TEST(ass.dependenciesCount == 0);
		}

		{
			AssetHeader ass("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", 13);
			CAGE_TEST(String(ass.cageName) == "cageAss");
			CAGE_TEST(ass.version > 0);
			CAGE_TEST(ass.flags == 0);
			CAGE_TEST(String(ass.textName) == "..rstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
			CAGE_TEST(ass.compressedSize == 0);
			CAGE_TEST(ass.originalSize == 0);
			CAGE_TEST(ass.scheme == 13);
			CAGE_TEST(ass.dependenciesCount == 0);
		}
	}
}
