#include <cstdlib>
#include <map>

#include "main.h"
#include <cage-core/filesystem.h>

#define FILE_BLOCKS (10)
#define BLOCK_SIZE (8*1024*1024)

void testFiles()
{
	CAGE_TESTCASE("files");

	{
		CAGE_TESTCASE("testdir has no subfolder files");
		holder<directoryListClass> fs = newDirectoryList("testdir/files");
		CAGE_TEST(fs);
		CAGE_TEST(!fs->valid());
	}

	holder<char> data = holder<char>((char*)malloc(BLOCK_SIZE), delegate<void(void*)>().bind<&free>());
	for (uint32 i = 0; i < BLOCK_SIZE; i++)
		(&*data)[i] = (char)(i % 26 + 'A');

	{
		CAGE_TESTCASE("write to file");
		holder<fileClass> f = newFile("testdir/files/1", fileMode(false, true));
		CAGE_TEST(f);
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
			f->write(data.get(), BLOCK_SIZE);
	}

	{
		CAGE_TESTCASE("read from file");
		holder<fileClass> f = newFile("testdir/files/1", fileMode(true, false));
		CAGE_TEST(f);
		CAGE_TEST(f->size() == (uint64)FILE_BLOCKS * (uint64)BLOCK_SIZE);
		holder <char> tmp = holder<char>((char*)malloc(BLOCK_SIZE), delegate<void(void*)>().bind<&free>());
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
		{
			f->read(&*tmp, BLOCK_SIZE);
			CAGE_TEST(detail::memcmp(&*tmp, &*data, BLOCK_SIZE) == 0);
		}
	}

	{
		CAGE_TESTCASE("create several files");
		holder<filesystemClass> fs = newFilesystem();
		fs->changeDir("testdir/files");
		CAGE_TEST(fs);
		for (uint32 i = 2; i <= 32; i++)
			fs->openFile(string(i), fileMode(false, true));
	}

	{
		CAGE_TESTCASE("list directory");
		holder<directoryListClass> fs = newDirectoryList("testdir/files");
		CAGE_TEST(fs);
		std::map<string, bool> mp;
		while (fs->valid())
		{
			mp[fs->name()] = true;
			fs->next();
		}
		CAGE_TEST(mp.size() == 32);
		for (uint32 i = 1; i <= 32; i++)
			CAGE_TEST(mp[i] == true);
	}

	{
		CAGE_TESTCASE("create files in subsequent folders");
		for (char a = 'a'; a < 'e'; a++)
		{
			for (char b = 'a'; b < 'e'; b++)
			{
				for (char c = 'a'; c < a; c++)
				{
					holder<filesystemClass> fs = newFilesystem();
					fs->changeDir("testdir/files");
					fs->changeDir(string(&a, 1));
					fs->changeDir(string(&b, 1));
					fs->openFile(string(&c, 1), fileMode(false, true));
				}
				holder<filesystemClass> fs = newFilesystem();
				fs->changeDir("testdir/files");
				fs->changeDir(string(&a, 1));
				fs->changeDir(string(&b, 1));
				fs->openFile("e", fileMode(false, true));
			}
			for (char b = 'e'; b < 'h'; b++)
			{
				holder<filesystemClass> fs = newFilesystem();
				fs->changeDir("testdir/files");
				fs->changeDir(string(&a, 1));
				fs->openFile(string(&b, 1), fileMode(false, true));
			}
		}
	}

	{
		CAGE_TESTCASE("list files in subsequent directory");
		holder<directoryListClass> fs = newDirectoryList("testdir/files/d/b");
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
		CAGE_TEST_THROWN(newFile("testdir/files/non-existing-file", fileMode(true, false)));
	}

	{
		CAGE_TESTCASE("lines");

		string bs = "ratata://omega.alt.com/blah/keee/jojo.armagedon";

		{
			holder<fileClass> f = newFile("testdir/files/lines", fileMode(false, true, true));
			string s = bs;
			while (!s.empty())
				f->writeLine(s.split("/"));
		}

		{
			holder<fileClass> f = newFile("testdir/files/lines", fileMode(true, false, true));
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
			CAGE_TEST(pathExists(source));
			CAGE_TEST(!pathExists(dest));
			pathMove(source, dest);
			CAGE_TEST(!pathExists(source));
			CAGE_TEST(pathExists(dest));
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
