#include "main.h"
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>

#include <cstdlib>
#include <set>

namespace
{
	constexpr uintPtr FILE_BLOCKS = 10;
	constexpr uintPtr BLOCK_SIZE = 8 * 1024 * 1024;

	void readInMemoryFile(Holder<File> &f)
	{
		CAGE_TEST(f->size() == BLOCK_SIZE);
		CAGE_TEST(f->tell() == 0);
		MemoryBuffer b = f->read(100);
		CAGE_TEST(f->tell() == 100);
		CAGE_TEST(b.size() == 100);
		CAGE_TEST(b.data()[0] == 'A');
		CAGE_TEST(b.data()[1] == 'B');
		CAGE_TEST(b.data()[26] == 'A');
		f->seek(BLOCK_SIZE - 10);
		CAGE_TEST(f->tell() == BLOCK_SIZE - 10);
		CAGE_TEST_THROWN(f->read(11));
	}
}

void testFiles()
{
	CAGE_TESTCASE("files");

	pathRemove("testdir");

	MemoryBuffer data(BLOCK_SIZE);
	for (uint32 i = 0; i < BLOCK_SIZE; i++)
		data.data()[i] = (char)(i % 26 + 'A');

	{
		CAGE_TESTCASE("write to file");
		Holder<File> f = writeFile("testdir/files/1");
		CAGE_TEST(f);
		for (uint32 i = 0; i < FILE_BLOCKS; i++)
			f->write(data);
	}

	{
		CAGE_TESTCASE("read from file");
		Holder<File> f = readFile("testdir/files/1");
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
			writeFile(pathJoin("testdir/files", stringizer() + i));
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
		writeFile("testdir/empty.txt");
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
			string sa({ &a, &a + 1 });
			for (char b = 'a'; b < 'e'; b++)
			{
				string sb({ &b, &b + 1 });
				for (char c = 'a'; c < a; c++)
				{
					string sc({ &c, &c + 1 });
					writeFile(pathJoin(pathJoin("testdir/files", sa), pathJoin(sb, sc)));
				}
				writeFile(pathJoin(pathJoin("testdir/files", sa), pathJoin(sb, "e")));
			}
			for (char b = 'e'; b < 'h'; b++)
			{
				string sb({ &b, &b + 1 });
				writeFile(pathJoin(pathJoin("testdir/files", sa), sb));
			}
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
		CAGE_TEST_THROWN(readFile("testdir/files/non-existing-file"));
	}

	{
		CAGE_TESTCASE("lines");

		const string bs = "ratata://omega.alt.com/blah/keee/jojo.armagedon";

		{
			Holder<File> f = writeFile("testdir/files/lines");
			string s = bs;
			while (!s.empty())
				f->writeLine(split(s, "/"));
		}

		{
			Holder<File> f = readFile("testdir/files/lines");
			string s = bs;
			for (string line; f->readLine(line);)
				CAGE_TEST(line == split(s, "/"));
			CAGE_TEST(s == "");
		}
	}

	{
		CAGE_TESTCASE("move file");

		{
			CAGE_TESTCASE("simple move");
			const string source = "testdir/files/1";
			const string dest = "testdir/moved/1";
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

	{
		CAGE_TESTCASE("in-memory files");
		{
			Holder<File> f = newFileBuffer(&data, FileMode(true, false));
			readInMemoryFile(f);
		}
		{
			Holder<File> f = newFileBuffer(PointerRange<char>(data));
			readInMemoryFile(f);
		}
		{
			Holder<File> f = newFileBuffer();
			f->write(data);
			CAGE_TEST(f->tell() == BLOCK_SIZE);
			f->seek(0);
			readInMemoryFile(f);
		}
	}
}
