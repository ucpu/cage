#include "main.h"
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/threadPool.h>

#include <set>

namespace
{
	constexpr const String directories[2] = {
		"testdir/files",
		"testdir/archive.zip"
	};

	void testWriteFile(const String &name, PointerRange<const char> data)
	{
		for (const String &dir : directories)
		{
			Holder<File> f = writeFile(pathJoin(dir, name));
			f->write(data);
			f->close();
		}
	}

	void testReadFile2(const String &name, Holder<PointerRange<char>> &a, Holder<PointerRange<char>> &b)
	{
		Holder<PointerRange<char>> *p[2] = { &a, &b };
		uint32 i = 0;
		for (const String &dir : directories)
		{
			Holder<File> f = readFile(pathJoin(dir, name));
			*p[i++] = f->readAll();
			f->close();
		}
	}

	void testBuffers(PointerRange<const char> a, PointerRange<const char> b)
	{
		CAGE_TEST(a.size() == b.size());
		CAGE_TEST(detail::memcmp(a.data(), b.data(), a.size()) == 0);
	}

	Holder<PointerRange<char>> testReadFile(const String &name)
	{
		Holder<PointerRange<char>> a, b;
		testReadFile2(name, a, b);
		testBuffers(a, b);
		return a;
	}

	void testCreateDirectories(const String &name)
	{
		for (const String &dir : directories)
			pathCreateDirectories(pathJoin(dir, name));
	}

	typedef std::set<std::pair<String, bool>> Listing;

	void testListDirectory2(const String &name, Listing &a, Listing &b)
	{
		Listing *p[2] = { &a, &b };
		uint32 i = 0;
		for (const String &dir : directories)
		{
			const auto list = pathListDirectory(pathJoin(dir, name));
			for (const String &n : list)
				p[i]->insert(std::make_pair<String, bool>(pathExtractFilename(n), any(pathType(n) & (PathTypeFlags::Directory | PathTypeFlags::Archive))));
			i++;
		}
	}

	PathTypeFlags testPathType(const String &name)
	{
		const PathTypeFlags a = pathType(pathJoin(directories[0], name));
		const PathTypeFlags b = pathType(pathJoin(directories[1], name));
		static constexpr PathTypeFlags msk = PathTypeFlags::File | PathTypeFlags::Directory;
		CAGE_TEST((a & msk) == (b & msk));
		return b;
	}

	Listing testListDirectory(const String &name)
	{
		Listing a, b;
		testListDirectory2(name, a, b);
		CAGE_TEST(a.size() == b.size());
		CAGE_TEST(a == b);
		for (const auto &n : a)
			testPathType(n.first);
		return a;
	}

	void testListRecursive(const String &name = "")
	{
		Listing l = testListDirectory(name);
		for (const auto &i : l)
		{
			if (i.second)
			{
				const String p = pathJoin(name, i.first);
				testListRecursive(p);
			}
		}
	}

	struct ConcurrentTester
	{
		static constexpr uint32 ThreadsCount = 4;

		Holder<Barrier> barrier = newBarrier(ThreadsCount);
		Holder<ThreadPool> threadPool = newThreadPool("tester_", ThreadsCount);
		Holder<PointerRange<char>> data;

		ConcurrentTester()
		{
			threadPool->function.bind<ConcurrentTester, &ConcurrentTester::threadEntry>(this);
			{ // generate random data
				MemoryBuffer buff;
				Serializer ser(buff);
				uint32 cnt = randomRange(10, 100);
				for (uint32 i = 0; i < cnt; i++)
					ser << randomRange(-1.0, 1.0);
				data = std::move(buff);
			}
		}

		void threadEntry(uint32 thrId, uint32)
		{
			for (uint32 iter = 0; iter < 10; iter++)
			{
				{ ScopeLock lck(barrier); }
				const String name = Stringizer() + "testdir/concurrent.zip/" + ((iter + thrId) % ThreadsCount) + ".bin";
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

void testArchives()
{
	CAGE_TESTCASE("archives");

	pathRemove("testdir");

	{
		CAGE_TESTCASE("create empty archive");
		pathCreateArchive(directories[1], "");
		CAGE_TEST(pathType(directories[1]) == (PathTypeFlags::File | PathTypeFlags::Archive));
		testListDirectory("");
	}

	Holder<PointerRange<char>> data1, data2, data3;
	{ // generate some random data
		Holder<PointerRange<char>> *data[3] = { &data1, &data2, &data3 };
		for (Holder<PointerRange<char>> *d : data)
		{
			MemoryBuffer buff;
			Serializer ser(buff);
			uint32 cnt = randomRange(10, 100);
			for (uint32 i = 0; i < cnt; i++)
				ser << randomRange(-1.0, 1.0);
			*d = std::move(buff);
		}
	}

	{
		CAGE_TESTCASE("first file");
		testWriteFile("first", data1);
		testReadFile("first");
		testListDirectory("");
	}

	{
		CAGE_TESTCASE("second file");
		testWriteFile("second", data2);
		testReadFile("second");
		testListDirectory("");
	}

	{
		CAGE_TESTCASE("third file (inside directory)");
		testWriteFile("dir/third", data3);
		testReadFile("dir/third");
		testListDirectory("");
		testListDirectory("dir");
		testListRecursive();
	}

	{
		CAGE_TESTCASE("create empty directories");
		testCreateDirectories("a/long/trip/to/a/small/angry/planet");
		testCreateDirectories("a/closed/and/common/orbit");
		testListRecursive();
	}

	{
		CAGE_TESTCASE("randomized files in random folders");
		for (uint32 i = 0; i < 3; i++)
		{
			String name = "rng";
			for (uint32 j = 0; j < 10; j++)
			{
				name += Stringizer() + randomRange(0, 9);
				if (randomChance() < 0.1)
					name += "/";
			}
			testWriteFile(name, data1);
		}
		testListRecursive();
	}

	{
		CAGE_TESTCASE("multiple write files at once");
		{
			Holder<File> ws[] = {
				writeFile(pathJoin(directories[0], "multi/1")),
				writeFile(pathJoin(directories[1], "multi/1")),
				writeFile(pathJoin(directories[0], "multi/2")),
				writeFile(pathJoin(directories[1], "multi/2")),
				writeFile(pathJoin(directories[0], "multi/3")),
				writeFile(pathJoin(directories[1], "multi/3"))
			};
			for (auto &f : ws)
				f->write(data1);
			for (auto &f : ws)
				f->write(data2);
			for (auto &f : ws)
				f->write(data3);
		}
		testReadFile("multi/1");
		testReadFile("multi/2");
		testReadFile("multi/3");
		testListRecursive();
	}

	{
		CAGE_TESTCASE("multiple read files at once");
		Holder<File> rs[] = {
			readFile(pathJoin(directories[0], "multi/1")),
			readFile(pathJoin(directories[1], "multi/1")),
			readFile(pathJoin(directories[0], "multi/2")),
			readFile(pathJoin(directories[1], "multi/2")),
			readFile(pathJoin(directories[0], "multi/3")),
			readFile(pathJoin(directories[1], "multi/3"))
		};
		for (auto &f : rs)
			f->read(data3.size());
		for (auto &f : rs)
			f->read(data2.size());
		for (auto &f : rs)
			f->read(data1.size());
	}

	{
		CAGE_TESTCASE("file seek & tell");
		const String path = pathJoin(directories[1], "multi/1");
		const PathTypeFlags type = pathType(path);
		CAGE_TEST(any(type & PathTypeFlags::File));
		CAGE_TEST(none(type & PathTypeFlags::Directory));
		CAGE_TEST(none(type & PathTypeFlags::Archive));
		Holder<File> f = readFile(path);
		f->seek(data1.size() + data2.size());
		Holder<PointerRange<char>> b3 = f->read(data3.size());
		CAGE_TEST(f->tell() == f->size());
		f->seek(data1.size());
		CAGE_TEST(f->tell() == data1.size());
		Holder<PointerRange<char>> b2 = f->read(data2.size());
		CAGE_TEST(f->tell() == data1.size() + data2.size());
		f->seek(0);
		CAGE_TEST(f->tell() == 0);
		Holder<PointerRange<char>> b1 = f->read(data1.size());
		CAGE_TEST(f->tell() == data1.size());
		f->close();
		testBuffers(b1, data1);
		testBuffers(b2, data2);
		testBuffers(b3, data3);
	}

	{
		CAGE_TESTCASE("read-write file");
		// first create the file
		for (const String &dir : directories)
		{
			Holder<File> f = writeFile(pathJoin(dir, "rw.bin"));
			f->write(data1);
			f->close();
		}
		// sanity check
		testListDirectory("");
		testReadFile("rw.bin");
		testReadFile("rw.bin"); // ensure that reading a file does not change it
		// first modification of the file
		for (const String &dir : directories)
		{
			Holder<File> f = newFile(pathJoin(dir, "rw.bin"), FileMode(true, true));
			CAGE_TEST(f->size() == data1.size());
			f->readAll();
			CAGE_TEST(f->tell() == data1.size());
			f->write(data2);
			CAGE_TEST(f->size() == data1.size() + data2.size());
			CAGE_TEST(f->tell() == data1.size() + data2.size());
			f->close();
		}
		// check
		testListDirectory("");
		testReadFile("rw.bin");
		// second modification of the file (including seek)
		for (const String &dir : directories)
		{
			Holder<File> f = newFile(pathJoin(dir, "rw.bin"), FileMode(true, true));
			f->seek(data2.size());
			f->write(data3);
			f->close();
		}
		// check
		testListDirectory("");
		testReadFile("rw.bin");
	}

	{
		CAGE_TESTCASE("remove a file");
		{
			Holder<void> tmp = detail::pathKeepOpen(directories[1]); // keep the archive open
			for (const String &dir : directories)
				pathRemove(pathJoin(dir, "multi/2"));
			testListRecursive(); // listing must work even while the changes to the archive are not yet written out
			CAGE_TEST(any(pathType(pathJoin(directories[1], "multi/2")) & PathTypeFlags::NotFound));
		}
		testListRecursive(); // and after they are written too
	}

	{
		CAGE_TESTCASE("move a file");
		{
			CAGE_TESTCASE("inside same archive");
			for (const String &dir : directories)
				pathMove(pathJoin(dir, "multi/3"), pathJoin(dir, "multi/4"));
			testListRecursive();
		}
		{
			CAGE_TESTCASE("from real filesystem to the archive");
			for (const String &dir : directories)
			{
				{
					Holder<File> f = writeFile("testdir/tmp");
					f->write(data2);
				}
				pathMove("testdir/tmp", pathJoin(dir, "moved"));
			}
			testReadFile("moved");
			testListRecursive();
		}
		{
			CAGE_TESTCASE("from the archive to real filesystem");
			for (const String &dir : directories)
				pathMove(pathJoin(dir, "multi/1"), "testdir/moved_up");
			testListRecursive();
		}
	}

	{
		CAGE_TESTCASE("opening an archive as regular file");
		const String arch = directories[1];
		{
			CAGE_TESTCASE("with open archive");
			Holder<void> tmp = detail::pathKeepOpen(arch); // keep the archive open
			CAGE_TEST_THROWN(readFile(arch));
		}
		{
			CAGE_TESTCASE("with closed archive");
			CAGE_TEST(readFile(arch));
		}
	}

	{
		CAGE_TESTCASE("manipulating a file that is opened as an archive");
		pathCreateArchive("testdir/arch2.zip");
		{
			CAGE_TESTCASE("remove");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.zip"); // open the archive
				CAGE_TEST_THROWN(pathRemove("testdir/arch2.zip"));
			}
			CAGE_TEST(any(pathType("testdir/arch2.zip") & PathTypeFlags::Archive)); // sanity check
		}
		{
			CAGE_TESTCASE("move from");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.zip"); // open the archive
				CAGE_TEST_THROWN(pathMove("testdir/arch2.zip", "testdir/arch3.zip"));
			}
			CAGE_TEST(any(pathType("testdir/arch2.zip") & PathTypeFlags::Archive)); // sanity check
			CAGE_TEST(none(pathType("testdir/arch3.zip") & PathTypeFlags::Archive)); // sanity check
		}
		{
			CAGE_TESTCASE("move to");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.zip"); // open the archive
				CAGE_TEST_THROWN(pathMove("testdir/archive.zip", "testdir/arch2.zip"));
			}
			CAGE_TEST(any(pathType("testdir/arch2.zip") & PathTypeFlags::Archive)); // sanity check
			CAGE_TEST(any(pathType("testdir/archive.zip") & PathTypeFlags::Archive)); // sanity check
		}
		{
			CAGE_TESTCASE("pathType while the file is in use");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.zip"); // open the archive
				CAGE_TEST(any(pathType("testdir/arch2.zip") & PathTypeFlags::Archive)); // try pythType on the archive itself
			}
			{
				writeFile("testdir/arch2.zip/abc.txt");
				Holder<File> f = writeFile("testdir/arch2.zip/def.txt");
				CAGE_TEST(any(pathType("testdir/arch2.zip") & PathTypeFlags::Archive)); // try pythType on the archive itself
				CAGE_TEST(any(pathType("testdir/arch2.zip/abc.txt") & PathTypeFlags::File)); // try pythType on a file inside the archive
				// todo CAGE_TEST(any(pathType("testdir/arch2.zip/def.txt") & PathTypeFlags::File)); // try pythType on a file inside the archive
			}
		}
		{
			CAGE_TESTCASE("allowed remove");
			pathRemove("testdir/arch2.zip"); // finally try to remove the file
			CAGE_TEST(none(pathType("testdir/arch2.zip") & PathTypeFlags::File)); // sanity check
		}
	}

	{
		CAGE_TESTCASE("concurrent randomized archive files");
		ConcurrentTester tester;
		for (uint32 i = 0; i < 10; i++)
		{
			CAGE_TESTCASE(Stringizer() + "iteration: " + i);
			tester.run();
		}
	}

	// todo lastChange
}
