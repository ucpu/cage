#include <set>

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/threadPool.h>

namespace
{
	constexpr const String directories[2] = { "testdir/files", "testdir/archive.carch" };

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

	using Listing = std::set<std::pair<String, bool>>;

	void testListDirectory2(const String &name, Listing &a, Listing &b)
	{
		Listing *p[2] = { &a, &b };
		uint32 i = 0;
		for (const String &dir : directories)
		{
			const auto list = pathListDirectory(pathJoin(dir, name));
			for (const String &n : list)
				p[i]->insert(std::make_pair<String, bool>(pathExtractFilename(n), pathIsDirectory(n)));
			i++;
		}
	}

	PathTypeFlags testPathType(const String &name)
	{
		const PathTypeFlags a = pathType(pathJoin(directories[0], name));
		const PathTypeFlags b = pathType(pathJoin(directories[1], name));
		static constexpr PathTypeFlags msk = ~PathTypeFlags::Archive;
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
		const Listing l = testListDirectory(name);
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
				{
					ScopeLock lck(barrier);
				}
				const String name = Stringizer() + "testdir/concurrent.carch/" + ((iter + thrId) % ThreadsCount) + ".bin";
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
					f->seek(randomRange((uint64)0, f->size()));
					f->write(data);
					f->close();
				}
			}
		}

		void run()
		{
			pathRemove("testdir/concurrent.carch");
			pathCreateArchiveCarch("testdir/concurrent.carch");
			threadPool->run();
		}
	};

	enum class MovePositionEnum
	{
		Nothing,
		File,
		Folder,
		Archive,
	};

	enum class MoveResultEnum
	{
		Error,
		Rename,
		Replace,
		Merge,
	};

	template<MovePositionEnum Source, MovePositionEnum Target, MoveResultEnum Result>
	void testMovesTableImpl()
	{
		const auto &prepare = [](MovePositionEnum pos, const String &pth)
		{
			switch (pos)
			{
				case MovePositionEnum::Nothing:
					break;
				case MovePositionEnum::File:
					writeFile(pth)->writeLine("file");
					break;
				case MovePositionEnum::Folder:
					pathCreateDirectories(pth);
					writeFile(pathJoin(pth, "aaa"))->writeLine("aaa");
					writeFile(pathJoin(pth, "bbb"))->writeLine("bbb");
					writeFile(pathJoin(pth, "ccc"))->writeLine("ccc");
					break;
				case MovePositionEnum::Archive:
					pathCreateArchiveCarch(pth);
					writeFile(pathJoin(pth, "aaa"))->writeLine("123");
					writeFile(pathJoin(pth, "ddd"))->writeLine("ddd");
					break;
			}
		};

		const auto &count = [](const String &pth) -> uint32
		{
			if (none(pathType(pth) & (PathTypeFlags::Directory | PathTypeFlags::Archive)))
				return 0;
			return pathListDirectory(pth).size();
		};

		static constexpr String source = "testdir/movingtester/source";
		static constexpr String target = "testdir/movingtester/target";
		pathRemove("testdir/movingtester");
		prepare(Source, source);
		prepare(Target, target);
		const uint32 srcCnt = count(source);
		const uint32 tgtCnt = count(target);

		if (Result == MoveResultEnum::Error)
		{
			CAGE_TEST_THROWN(pathMove(source, target));
		}
		else
			pathMove(source, target);

		if (Result != MoveResultEnum::Error)
			CAGE_TEST(pathType(source) == PathTypeFlags::NotFound);

		switch (Result)
		{
			case MoveResultEnum::Error:
				break;
			case MoveResultEnum::Rename:
			case MoveResultEnum::Replace:
			{
				switch (Source)
				{
					case MovePositionEnum::Nothing:
						break;
					case MovePositionEnum::File:
						CAGE_TEST(pathType(target) == PathTypeFlags::File);
						CAGE_TEST(readFile(target)->readLine() == "file");
						break;
					case MovePositionEnum::Folder:
						CAGE_TEST(pathType(target) == PathTypeFlags::Directory);
						CAGE_TEST(count(target) == 3);
						CAGE_TEST(pathType(pathJoin(target, "file")) == PathTypeFlags::NotFound);
						CAGE_TEST(readFile(pathJoin(target, "aaa"))->readLine() == "aaa");
						CAGE_TEST(readFile(pathJoin(target, "bbb"))->readLine() == "bbb");
						CAGE_TEST(readFile(pathJoin(target, "ccc"))->readLine() == "ccc");
						CAGE_TEST(pathType(pathJoin(target, "ddd")) == PathTypeFlags::NotFound);
						break;
					case MovePositionEnum::Archive:
						CAGE_TEST(any(pathType(target) & PathTypeFlags::Archive));
						CAGE_TEST(count(target) == 2);
						CAGE_TEST(pathType(pathJoin(target, "file")) == PathTypeFlags::NotFound);
						CAGE_TEST(readFile(pathJoin(target, "aaa"))->readLine() == "123");
						CAGE_TEST(pathType(pathJoin(target, "bbb")) == PathTypeFlags::NotFound);
						CAGE_TEST(pathType(pathJoin(target, "ccc")) == PathTypeFlags::NotFound);
						CAGE_TEST(readFile(pathJoin(target, "ddd"))->readLine() == "ddd");
						break;
				}
			}
			break;
			case MoveResultEnum::Merge:
			{
				CAGE_TEST(any(pathType(target) & (PathTypeFlags::Archive | PathTypeFlags::Directory)));
				CAGE_TEST(pathType(pathJoin(target, "file")) == PathTypeFlags::NotFound);
				switch (Source)
				{
					case MovePositionEnum::Nothing:
					case MovePositionEnum::File:
						break;
					case MovePositionEnum::Folder:
						CAGE_TEST(readFile(pathJoin(target, "aaa"))->readLine() == "aaa");
						CAGE_TEST(readFile(pathJoin(target, "bbb"))->readLine() == "bbb");
						CAGE_TEST(readFile(pathJoin(target, "ccc"))->readLine() == "ccc");
						break;
					case MovePositionEnum::Archive:
						CAGE_TEST(readFile(pathJoin(target, "aaa"))->readLine() == "123");
						CAGE_TEST(readFile(pathJoin(target, "ddd"))->readLine() == "ddd");
						break;
				}
				switch (Target)
				{
					case MovePositionEnum::Nothing:
					case MovePositionEnum::File:
						break;
					case MovePositionEnum::Folder:
						if (Source == Target)
							CAGE_TEST(count(target) == 3); // aaa, bbb, ccc
						break;
					case MovePositionEnum::Archive:
						if (Source == Target)
							CAGE_TEST(count(target) == 2); // aaa, ddd
						break;
				}
				if (Source != Target)
					CAGE_TEST(count(target) == 4); // aaa, bbb, ccc, ddd
			}
			break;
		}
	}

	void testMovesTable()
	{
		CAGE_TESTCASE("pathMove table");
		/*
			to	|	nothing	file	folder	archive
		from----+-----------------------------------
		nothing	|	error	error	error	error
		file	|	rename	replace	error	replace
		folder	|	rename	error	merge	merge
		archive	|	rename	replace	merge	merge
		*/
		using P = MovePositionEnum;
		using R = MoveResultEnum;
		testMovesTableImpl<P::Nothing, P::Nothing, R::Error>();
		testMovesTableImpl<P::Nothing, P::File, R::Error>();
		testMovesTableImpl<P::Nothing, P::Folder, R::Error>();
		testMovesTableImpl<P::Nothing, P::Archive, R::Error>();
		testMovesTableImpl<P::File, P::Nothing, R::Rename>();
		testMovesTableImpl<P::File, P::File, R::Replace>();
		testMovesTableImpl<P::File, P::Folder, R::Error>();
		testMovesTableImpl<P::File, P::Archive, R::Replace>();
		testMovesTableImpl<P::Folder, P::Nothing, R::Rename>();
		testMovesTableImpl<P::Folder, P::File, R::Error>();
		testMovesTableImpl<P::Folder, P::Folder, R::Merge>();
		testMovesTableImpl<P::Folder, P::Archive, R::Merge>();
		testMovesTableImpl<P::Archive, P::Nothing, R::Rename>();
		testMovesTableImpl<P::Archive, P::File, R::Replace>();
		testMovesTableImpl<P::Archive, P::Folder, R::Merge>();
		testMovesTableImpl<P::Archive, P::Archive, R::Merge>();
	}
}

void testArchivesCarch()
{
	CAGE_TESTCASE("archives CARCH");

	pathRemove("testdir");

	{
		CAGE_TESTCASE("create empty archive");
		pathCreateDirectories(directories[0]);
		pathCreateArchiveCarch(directories[1]);
		CAGE_TEST(pathType(directories[1]) == (PathTypeFlags::File | PathTypeFlags::Archive));
		testListDirectory("");
	}

	{
		CAGE_TESTCASE("create archive where name already exists");
		CAGE_TEST_THROWN(pathCreateArchiveCarch(directories[1])); // already existing archive
		pathCreateDirectories("testdir/archiveplaceholder");
		CAGE_TEST_THROWN(pathCreateArchiveCarch("testdir/archiveplaceholder")); // already existing folder
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
			Holder<File> ws[] = { writeFile(pathJoin(directories[0], "multi/1")), writeFile(pathJoin(directories[1], "multi/1")), writeFile(pathJoin(directories[0], "multi/2")), writeFile(pathJoin(directories[1], "multi/2")), writeFile(pathJoin(directories[0], "multi/3")), writeFile(pathJoin(directories[1], "multi/3")) };
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
		Holder<File> rs[] = { readFile(pathJoin(directories[0], "multi/1")), readFile(pathJoin(directories[1], "multi/1")), readFile(pathJoin(directories[0], "multi/2")), readFile(pathJoin(directories[1], "multi/2")), readFile(pathJoin(directories[0], "multi/3")), readFile(pathJoin(directories[1], "multi/3")) };
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
		CAGE_TESTCASE("remove non-existent file");
		for (const String &dir : directories)
			pathRemove(pathJoin(dir, "non-existent-file-blee-bloo")); // does not throw
		testListRecursive();
	}

	{
		CAGE_TESTCASE("list invalid folder");
		for (const String &dir : directories)
		{
			CAGE_TEST(pathType(pathJoin(dir, "rw.bin")) == PathTypeFlags::File);
			CAGE_TEST_THROWN(pathListDirectory(pathJoin(dir, "rw.bin")));
			CAGE_TEST(pathType(pathJoin(dir, "non-existent-dir-jupii-jou")) == PathTypeFlags::NotFound);
			CAGE_TEST_THROWN(pathListDirectory(pathJoin(dir, "non-existent-dir-jupii-jou")));
		}
		testListRecursive();
	}

	{
		CAGE_TESTCASE("list folder with none/one/multiple files");
		for (const String &dir : directories)
		{
			const String pth = pathJoin(dir, "listcounts");
			pathCreateDirectories(pth);
			CAGE_TEST(pathListDirectory(pth).empty());
			writeFile(pathJoin(pth, "one"))->writeLine("helo");
			CAGE_TEST(pathListDirectory(pth).size() == 1);
			writeFile(pathJoin(pth, "two"))->writeLine("hello");
			CAGE_TEST(pathListDirectory(pth).size() == 2);
		}
		testListRecursive();
	}

	{
		CAGE_TESTCASE("move");
		{
			CAGE_TESTCASE("inside same archive");
			for (const String &dir : directories)
				pathMove(pathJoin(dir, "multi/3"), pathJoin(dir, "multi/4")); // file
			testListRecursive();
			for (const String &dir : directories)
				pathMove(pathJoin(dir, "multi"), pathJoin(dir, "multi2")); // folder
			testListRecursive();
			for (const String &dir : directories)
				pathMove(pathJoin(dir, "multi2"), pathJoin(dir, "multi")); // folder back
			testListRecursive();
		}
		{
			CAGE_TESTCASE("from real filesystem to the archive");
			for (const String &dir : directories)
			{
				writeFile("testdir/tmp")->write(data2);
				pathMove("testdir/tmp", pathJoin(dir, "moved")); // file
			}
			testReadFile("moved");
			testListRecursive();
			for (const String &dir : directories)
			{
				writeFile("testdir/a/b/c")->write(data2);
				pathMove("testdir/a", pathJoin(dir, "b")); // folder
			}
			testListRecursive();
		}
		{
			CAGE_TESTCASE("from the archive to real filesystem");
			for (const String &dir : directories)
				pathMove(pathJoin(dir, "multi/1"), "testdir/moved_up");
			testListRecursive();
			for (const String &dir : directories)
			{
				writeFile(pathJoin(dir, "c/d/e"))->write(data2);
				pathMove(pathJoin(dir, "c"), "testdir/d"); // folder
			}
			testListRecursive();
		}
		{
			CAGE_TESTCASE("move folder onto archive");
			for (const String &dir : directories)
			{
				writeFile("testdir/g/txt")->write(data2);
				pathMove("testdir/g", dir);
			}
			testListRecursive();
			pathCreateArchiveCarch("testdir/arch3.carch");
			writeFile("testdir/arch3.carch/file.txt")->writeLine("haha");
			pathCreateDirectories("testdir/arch3");
			pathMove("testdir/arch3.carch", "testdir/arch3");
			CAGE_TEST(pathType("testdir/arch3.carch") == PathTypeFlags::NotFound);
			CAGE_TEST(pathType("testdir/arch3/file.txt") == PathTypeFlags::File);
		}
		{
			CAGE_TESTCASE("move archive into folder");
			pathCreateArchiveCarch("testdir/movingarch.carch");
			writeFile("testdir/movingarch.carch/file.txt")->writeLine("haha");
			pathCreateDirectories("testdir/movingdest");
			CAGE_TEST(pathType("testdir/movingarch.carch") == (PathTypeFlags::File | PathTypeFlags::Archive));
			CAGE_TEST(pathType("testdir/movingarch.carch/file.txt") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/movingdest") == PathTypeFlags::Directory);
			pathMove("testdir/movingarch.carch", "testdir/movingdest/movedarch.carch");
			CAGE_TEST(pathType("testdir/movingarch.carch") == PathTypeFlags::NotFound);
			CAGE_TEST(pathType("testdir/movingdest/movedarch.carch") == (PathTypeFlags::File | PathTypeFlags::Archive));
			CAGE_TEST(pathType("testdir/movingdest/movedarch.carch/file.txt") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/movingdest") == PathTypeFlags::Directory);
		}
	}

	testMovesTable();

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
		pathCreateArchiveCarch("testdir/arch2.carch");
		{
			CAGE_TESTCASE("remove");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.carch"); // open the archive
				CAGE_TEST_THROWN(pathRemove("testdir/arch2.carch"));
			}
			CAGE_TEST(any(pathType("testdir/arch2.carch") & PathTypeFlags::Archive)); // sanity check
		}
		{
			CAGE_TESTCASE("move from");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.carch"); // open the archive
				CAGE_TEST_THROWN(pathMove("testdir/arch2.carch", "testdir/arch3.carch"));
			}
			CAGE_TEST(any(pathType("testdir/arch2.carch") & PathTypeFlags::Archive)); // sanity check
			CAGE_TEST(none(pathType("testdir/arch3.carch") & PathTypeFlags::Archive)); // sanity check
		}
		{
			CAGE_TESTCASE("move to");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.carch"); // open the archive
				CAGE_TEST_THROWN(pathMove("testdir/archive.carch", "testdir/arch2.carch"));
			}
			CAGE_TEST(any(pathType("testdir/arch2.carch") & PathTypeFlags::Archive)); // sanity check
			CAGE_TEST(any(pathType("testdir/archive.carch") & PathTypeFlags::Archive)); // sanity check
		}
		{
			CAGE_TESTCASE("pathType while the file is in use");
			{
				Holder<void> tmp = detail::pathKeepOpen("testdir/arch2.carch"); // open the archive
				CAGE_TEST(any(pathType("testdir/arch2.carch") & PathTypeFlags::Archive)); // try pythType on the archive itself
			}
			{
				writeFile("testdir/arch2.carch/abc.txt");
				Holder<File> f = writeFile("testdir/arch2.carch/def.txt");
				CAGE_TEST(any(pathType("testdir/arch2.carch") & PathTypeFlags::Archive)); // try pythType on the archive itself
				CAGE_TEST(any(pathType("testdir/arch2.carch/abc.txt") & PathTypeFlags::File)); // try pythType on a file inside the archive
				// todo CAGE_TEST(any(pathType("testdir/arch2.carch/def.txt") & PathTypeFlags::File)); // try pythType on an opened file inside the archive
			}
		}
		{
			CAGE_TESTCASE("allowed remove");
			pathRemove("testdir/arch2.carch"); // finally try to remove the file
			CAGE_TEST(none(pathType("testdir/arch2.carch") & PathTypeFlags::File)); // sanity check
		}
	}

	{
		CAGE_TESTCASE("keep open");
		pathCreateDirectories("testdir/keep-open/folder");
		writeFile("testdir/keep-open/file")->writeLine("haha");
		pathCreateArchiveCarch("testdir/keep-open/archive.carch");
		CAGE_TEST(detail::pathKeepOpen("testdir/keep-open/folder"));
		CAGE_TEST(detail::pathKeepOpen("testdir/keep-open/file"));
		CAGE_TEST(detail::pathKeepOpen("testdir/keep-open/archive.carch"));
		CAGE_TEST_THROWN(detail::pathKeepOpen("testdir/keep-open/non-existent")); // the path must already exist
		CAGE_TEST(pathType("testdir/keep-open/folder") == (PathTypeFlags::Directory));
		CAGE_TEST(pathType("testdir/keep-open/file") == (PathTypeFlags::File));
		CAGE_TEST(pathType("testdir/keep-open/archive.carch") == (PathTypeFlags::File | PathTypeFlags::Archive));
		CAGE_TEST(pathType("testdir/keep-open/non-existent") == (PathTypeFlags::NotFound)); // the path may not be created
	}

	{
		CAGE_TESTCASE("open same file multiple times");
		pathCreateArchiveCarch("testdir/arch5.carch");
		{
			Holder<File> f1 = writeFile("testdir/arch5.carch/samefile.txt");
			CAGE_TEST(f1);
			CAGE_TEST_THROWN(readFile("testdir/arch5.carch/samefile.txt"));
		}
		{
			Holder<File> f1 = readFile("testdir/arch5.carch/samefile.txt");
			CAGE_TEST(f1);
			CAGE_TEST_THROWN(readFile("testdir/arch5.carch/samefile.txt"));
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
}
