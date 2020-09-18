#include "main.h"
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>

#include <cstdlib>
#include <set>

#define FILE_BLOCKS (10)
#define BLOCK_SIZE (8*1024*1024)

void testFiles()
{
	CAGE_TESTCASE("files");

	pathRemove("testdir");

	MemoryBuffer data(BLOCK_SIZE);
	for (uint32 i = 0; i < BLOCK_SIZE; i++)
		data.data()[i] = (char)(i % 26 + 'A');

	{
		CAGE_TESTCASE("write to file");
		Holder<File> f = newFile("testdir/files/1", FileMode(false, true));
		CAGE_TEST(f);
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
			f->write(data);
	}

	{
		CAGE_TESTCASE("read from file");
		Holder<File> f = newFile("testdir/files/1", FileMode(true, false));
		CAGE_TEST(f);
		CAGE_TEST(f->size() == (uint64)FILE_BLOCKS * (uint64)BLOCK_SIZE);
		MemoryBuffer tmp(BLOCK_SIZE);
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
		{
			f->read(tmp);
			CAGE_TEST(detail::memcmp(tmp.data(), data.data(), BLOCK_SIZE) == 0);
		}
	}

	{
		CAGE_TESTCASE("create several files");
		for (uint32 i = 2; i <= 32; i++)
			newFile(pathJoin("testdir/files", stringizer() + i), FileMode(false, true));
	}

	{
		CAGE_TESTCASE("list directory");
		Holder<DirectoryList> fs = newDirectoryList("testdir/files");
		CAGE_TEST(fs);
		std::set<string> mp;
		while (fs->valid())
		{
			mp.insert(fs->name());
			fs->next();
		}
		CAGE_TEST(mp.size() == 32);
		for (uint32 i = 1; i <= 32; i++)
			CAGE_TEST(mp.count(stringizer() + i));
	}

	{
		CAGE_TESTCASE("list directory - test directories");
		Holder<DirectoryList> fs = newDirectoryList("testdir");
		CAGE_TEST(fs);
		bool found = false;
		while (fs->valid())
		{
			if (fs->name() == "files")
			{
				CAGE_TEST(fs->isDirectory());
				found = true;
			}
			fs->next();
		}
		CAGE_TEST(found);
	}

	{
		CAGE_TESTCASE("list directory - test empty file");
		newFile("testdir/empty.txt", FileMode(false, true));
		Holder<DirectoryList> fs = newDirectoryList("testdir");
		CAGE_TEST(fs);
		bool found = false;
		while (fs->valid())
		{
			if (fs->name() == "empty.txt")
			{
				CAGE_TEST(any(fs->type() & PathTypeFlags::File));
				CAGE_TEST(!fs->isDirectory());
				found = true;
			}
			fs->next();
		}
		CAGE_TEST(found);
	}

	{
		CAGE_TESTCASE("create files in subsequent folders");
		for (char a = 'a'; a < 'e'; a++)
		{
			for (char b = 'a'; b < 'e'; b++)
			{
				for (char c = 'a'; c < a; c++)
					newFile(pathJoin(pathJoin("testdir/files", string(&a, 1)), pathJoin(string(&b, 1), string(&c, 1))), FileMode(false, true));
				newFile(pathJoin(pathJoin("testdir/files", string(&a, 1)), pathJoin(string(&b, 1), "e")), FileMode(false, true));
			}
			for (char b = 'e'; b < 'h'; b++)
				newFile(pathJoin(pathJoin("testdir/files", string(&a, 1)), string(&b, 1)), FileMode(false, true));
		}
	}

	{
		CAGE_TESTCASE("list files in subsequent directory");
		Holder<DirectoryList> fs = newDirectoryList("testdir/files/d/b");
		uint32 cnt = 0;
		while (fs->valid())
		{
			cnt++;
			fs->next();
		}
		CAGE_TEST(cnt == 4, cnt);
	}

	{
		CAGE_TESTCASE("non-existing file");
		CAGE_TEST_THROWN(newFile("testdir/files/non-existing-file", FileMode(true, false)));
	}

	{
		CAGE_TESTCASE("lines");

		const string bs = "ratata://omega.alt.com/blah/keee/jojo.armagedon";

		{
			Holder<File> f = writeFile("testdir/files/lines");
			string s = bs;
			while (!s.empty())
				f->writeLine(s.split("/"));
		}

		{
			Holder<File> f = readFile("testdir/files/lines");
			string s = bs;
			for (string line; f->readLine(line);)
				CAGE_TEST(line == s.split("/"));
			CAGE_TEST(s == "");
		}
	}

	{
		CAGE_TESTCASE("move file");

		{
			CAGE_TESTCASE("simple move");
			string source = "testdir/files/1";
			string dest = "testdir/moved/1";
			CAGE_TEST(pathIsFile(source));
			CAGE_TEST(!pathIsFile(dest));
			pathMove(source, dest);
			CAGE_TEST(!pathIsFile(source));
			CAGE_TEST(pathIsFile(dest));
		}

		{
			CAGE_TESTCASE("move to non-existing directory");
			pathMove("testdir/moved/1", "testdir/moved/non-exist/1");
		}

		{
			CAGE_TESTCASE("move non-existing file");
			CAGE_TEST_THROWN(pathMove("testdir/moved/non-existing-file", "testdir/moved/1"));
		}
	}
}
