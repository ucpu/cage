#include "main.h"

#include <cage-core/files.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>

namespace
{
	constexpr uint64 MB = 1024 * 1024;
	uint64 tmp[751]; // not a multiple of 1024

	MemoryBuffer generateBuffer(uint64 size, uint64 seed)
	{
		MemoryBuffer buff;
		buff.resize(size);
		for (uint64 &t : tmp)
			t = seed++;
		uint64 p = 0;
		while (true)
		{
			uint64 s = size - p;
			s = min(s, (uint64)sizeof(tmp));
			if (s == 0)
				break;
			detail::memcpy(buff.data() + p, tmp, s);
			p += s;
		}
		return buff;
	}

	void compareBuffer(PointerRange<const char> buff, uint64 size, uint64 seed)
	{
		CAGE_TEST(buff.size() == size);
		for (uint64 &t : tmp)
			t = seed++;
		uint64 p = 0;
		while (true)
		{
			uint64 s = size - p;
			s = min(s, (uint64)sizeof(tmp));
			if (s == 0)
				break;
			CAGE_TEST(detail::memcmp(buff.data() + p, tmp, s) == 0);
			p += s;
		}
		CAGE_TEST(p == size);
	}
}

void testArchivesBigFiles()
{
	CAGE_TESTCASE("archives big files");

	pathRemove("testdir");

	{
		CAGE_TESTCASE("big file in several chunks");
		{
			Holder<File> f = writeFile("testdir/bigfilechunks");
			for (uint32 i = 0; i < 60; i++)
				f->write(generateBuffer(100 * MB, 321 * i + 542));
			CAGE_TEST(f->size() == 60 * 100 * MB);
			f->close();
		}
		{
			Holder<File> f = readFile("testdir/bigfilechunks");
			CAGE_TEST(f->size() == 60 * 100 * MB);
			CAGE_TEST(f->tell() == 0);
			for (uint32 i = 0; i < 60; i++)
			{
				const auto buff = f->read(100 * MB);
				compareBuffer(buff, 100 * MB, 321 * i + 542);
			}
			CAGE_TEST(f->size() == 60 * 100 * MB);
			CAGE_TEST(f->tell() == 60 * 100 * MB);
			f->close();
		}
		pathRemove("testdir");
	}

	{
		CAGE_TESTCASE("big file in one pass");
#ifdef CAGE_ARCHITECTURE_64
		{
			MemoryBuffer buff = generateBuffer(6 * 1024 * MB, 123'456);
			compareBuffer(buff, 6 * 1024 * MB, 123'456); // sanity test
			Holder<File> f = writeFile("testdir/bigfileonepass");
			f->write(buff);
			CAGE_TEST(f->size() == 6 * 1024 * MB);
			f->close();
		}
		{
			Holder<File> f = readFile("testdir/bigfileonepass");
			CAGE_TEST(f->size() == 6 * 1024 * MB);
			CAGE_TEST(f->tell() == 0);
			auto buff = f->readAll();
			CAGE_TEST(f->size() == 6 * 1024 * MB);
			CAGE_TEST(f->tell() == 6 * 1024 * MB);
			f->close();
			compareBuffer(buff, 6 * 1024 * MB, 123'456);
		}
		pathRemove("testdir");
#else
		CAGE_LOG(SeverityEnum::Warning, "tests", "this testcase is skipped on 32-bit platform");
#endif // CAGE_ARCHITECTURE_64
	}
}
