#include <cstdlib>
#include <set>

#include "main.h"
#include <cage-core/files.h>
#include <cage-core/fileUtils.h>
#include <cage-core/memoryBuffer.h>

#define FILE_BLOCKS (10)
#define BLOCK_SIZE (8*1024*1024)

void testFiles()
{
	CAGE_TESTCASE("files");

	pathRemove("testdir");

	memoryBuffer data(BLOCK_SIZE);
	for (uint32 i = 0; i < BLOCK_SIZE; i++)
		data.data()[i] = (char)(i % 26 + 'A');

	{
		CAGE_TESTCASE("write to fileHandle");
		holder<fileHandle> f = newFile("testdir/files/1", fileMode(false, true));
		CAGE_TEST(f);
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
			f->write(data.data(), BLOCK_SIZE);
	}

	{
		CAGE_TESTCASE("read from fileHandle");
		holder<fileHandle> f = newFile("testdir/files/1", fileMode(true, false));
		CAGE_TEST(f);
		CAGE_TEST(f->size() == (uint64)FILE_BLOCKS * (uint64)BLOCK_SIZE);
		memoryBuffer tmp(BLOCK_SIZE);
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
		{
			f->read(tmp.data(), BLOCK_SIZE);
			CAGE_TEST(detail::memcmp(tmp.data(), data.data(), BLOCK_SIZE) == 0);
		}
	}

	{
		CAGE_TESTCASE("create several files");
		holder<filesystem> fs = newFilesystem();
		fs->changeDir("testdir/files");
		CAGE_TEST(fs);
		for (uint32 i = 2; i <= 32; i++)
			fs->openFile(string(i), fileMode(false, true));
	}

	{
		CAGE_TESTCASE("list directory");
		holder<directoryList> fs = newDirectoryList("testdir/files");
		CAGE_TEST(fs);
		std::set<string> mp;
		while (fs->valid())
		{
			mp.insert(fs->name());
			fs->next();
		}
		CAGE_TEST(mp.size() == 32);
		for (uint32 i = 1; i <= 32; i++)
			CAGE_TEST(mp.count(string(i)));
	}

	{
		CAGE_TESTCASE("list directory - test directories");
		holder<directoryList> fs = newDirectoryList("testdir");
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
		CAGE_TESTCASE("list directory - test empty fileHandle");
		{
			auto f = newFile("testdir/empty.txt", fileMode(false, true));
			//f->writeLine("haha");
		}
		holder<directoryList> fs = newDirectoryList("testdir");
		CAGE_TEST(fs);
		bool found = false;
		while (fs->valid())
		{
			if (fs->name() == "empty.txt")
			{
				CAGE_TEST(any(fs->type() & pathTypeFlags::File));
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
				{
					holder<filesystem> fs = newFilesystem();
					fs->changeDir("testdir/files");
					fs->changeDir(string(&a, 1));
					fs->changeDir(string(&b, 1));
					fs->openFile(string(&c, 1), fileMode(false, true));
				}
				holder<filesystem> fs = newFilesystem();
				fs->changeDir("testdir/files");
				fs->changeDir(string(&a, 1));
				fs->changeDir(string(&b, 1));
				fs->openFile("e", fileMode(false, true));
			}
			for (char b = 'e'; b < 'h'; b++)
			{
				holder<filesystem> fs = newFilesystem();
				fs->changeDir("testdir/files");
				fs->changeDir(string(&a, 1));
				fs->openFile(string(&b, 1), fileMode(false, true));
			}
		}
	}

	{
		CAGE_TESTCASE("list files in subsequent directory");
		holder<directoryList> fs = newDirectoryList("testdir/files/d/b");
		uint32 cnt = 0;
		while (fs->valid())
		{
			cnt++;
			fs->next();
		}
		CAGE_TEST(cnt == 4, cnt);
	}

	{
		CAGE_TESTCASE("non-existing fileHandle");
		CAGE_TEST_THROWN(newFile("testdir/files/non-existing-fileHandle", fileMode(true, false)));
	}

	{
		CAGE_TESTCASE("lines");

		const string bs = "ratata://omega.alt.com/blah/keee/jojo.armagedon";

		{
			holder<fileHandle> f = newFile("testdir/files/lines", fileMode(false, true));
			string s = bs;
			while (!s.empty())
				f->writeLine(s.split("/"));
		}

		{
			holder<fileHandle> f = newFile("testdir/files/lines", fileMode(true, false));
			string s = bs;
			for (string line; f->readLine(line);)
				CAGE_TEST(line == s.split("/"));
			CAGE_TEST(s == "");
		}
	}

	{
		CAGE_TESTCASE("move fileHandle");

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
			CAGE_TESTCASE("move non-existing fileHandle");
			CAGE_TEST_THROWN(pathMove("testdir/moved/non-existing-fileHandle", "testdir/moved/1"));
		}
	}
}
