#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/threadPool.h>

namespace
{
	template<uint32 Mode>
	struct ConcurrentTester
	{
		static constexpr uint32 ThreadsCount = 4;

		Holder<Barrier> barrier = newBarrier(ThreadsCount);
		Holder<ThreadPool> threadPool = newThreadPool("tester_", ThreadsCount);
		MemoryBuffer data;

		ConcurrentTester()
		{
			threadPool->function.bind<ConcurrentTester, &ConcurrentTester::threadEntry>(this);
			{ // generate random data
				Serializer ser(data);
				const uint32 cnt = randomRange(10, 100);
				for (uint32 i = 0; i < cnt; i++)
					ser << randomRange(-1.0, 1.0);
			}
		}

		void threadEntry(uint32 thrId, uint32)
		{
			pathCreateArchive(Stringizer() + "testdir/concurrent.zip/" + thrId + ".zip");
			for (uint32 iter = 0; iter < 10; iter++)
			{
				{
					ScopeLock lck(barrier);
				}
				const String name = Mode == 0 ? Stringizer() + "testdir/concurrent.zip/" + ((iter + thrId) % ThreadsCount) + ".zip/" + randomRange(0, 3) + ".bin" : Stringizer() + "testdir/concurrent.zip/" + randomRange(0, 3) + ".zip/" + ((iter + thrId) % ThreadsCount) + ".bin";
				const PathTypeFlags pf = pathType(name);
				if (any(pf & PathTypeFlags::File))
				{
					if (randomChance() < 0.2)
						pathRemove(name);
					else
					{
						Holder<File> f = readFile(name);
						f->readAll();
					}
				}
				if (none(pf & PathTypeFlags::File) || randomChance() < 0.3)
				{
					Holder<File> f = writeFile(name);
					f->seek(randomRange((uintPtr)0, f->size()));
					f->write(data);
					f->close();
				}
			}
		}

		void run()
		{
			pathRemove("testdir/concurrent.zip");
			pathCreateArchive("testdir/concurrent.zip");
			threadPool->run();
		}
	};
}

void testArchivesRecursion()
{
	CAGE_TESTCASE("archives recursion");

	pathRemove("testdir");

	{
		CAGE_TESTCASE("create archive inside archive inside archive ...");
		String p = "testdir";
		for (uint32 i = 0; i < 5; i++)
		{
			p = pathJoin(p, Stringizer() + i + ".zip");
			pathCreateArchive(p);
		}
		CAGE_TEST(none(pathType("testdir") & PathTypeFlags::Archive));
		CAGE_TEST(any(pathType("testdir") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip") & PathTypeFlags::Directory));
	}

	{
		CAGE_TESTCASE("create file inside each archive");
		String p = "testdir";
		for (uint32 i = 0; i < 5; i++)
		{
			p = pathJoin(p, Stringizer() + i + ".zip");
			Holder<File> f = writeFile(pathJoin(p, "welcome"));
			f->writeLine("hello");
			f->close();
		}
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::File));
	}

	{
		CAGE_TESTCASE("read file inside each archive");
		String p = "testdir";
		for (uint32 i = 0; i < 5; i++)
		{
			p = pathJoin(p, Stringizer() + i + ".zip");
			Holder<File> f = readFile(pathJoin(p, "welcome"));
			String l;
			CAGE_TEST(f->readLine(l));
			CAGE_TEST(l == "hello");
			f->close();
		}
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::File));
	}

	{
		CAGE_TESTCASE("opening an archive (inside archive) as regular file");
		{
			CAGE_TESTCASE("with open archive");
			String p = "testdir";
			for (uint32 i = 0; i < 5; i++)
			{
				p = pathJoin(p, Stringizer() + i + ".zip");
				Holder<void> tmp = detail::pathKeepOpen(p); // ensure the archive is open
				CAGE_TEST_THROWN(readFile(p));
			}
		}
		{
			CAGE_TESTCASE("with closed archive");
			String p = "testdir";
			for (uint32 i = 0; i < 5; i++)
			{
				p = pathJoin(p, Stringizer() + i + ".zip");
				CAGE_TEST(readFile(p));
			}
		}
	}

	{
		CAGE_TESTCASE("concurrent randomized recursive archive files");
		{
			ConcurrentTester<0> tester;
			for (uint32 i = 0; i < 10; i++)
			{
				CAGE_TESTCASE(Stringizer() + "iteration: " + i);
				tester.run();
			}
		}
		{
			ConcurrentTester<1> tester;
			for (uint32 i = 0; i < 10; i++)
			{
				CAGE_TESTCASE(Stringizer() + "iteration: " + i);
				tester.run();
			}
		}
	}
}
