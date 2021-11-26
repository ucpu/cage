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
		Holder<PointerRange<char>> b = f->read(100);
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
		CAGE_TESTCASE("readAll from file");
		Holder<File> f = readFile("testdir/files/1");
		CAGE_TEST(f);
		CAGE_TEST(f->size() == (uint64)FILE_BLOCKS * (uint64)BLOCK_SIZE);
		auto tmp = f->readAll();
		CAGE_TEST(tmp.size() == (uint64)FILE_BLOCKS * (uint64)BLOCK_SIZE);
	}

	{
		CAGE_TESTCASE("create several files");
		for (uint32 i = 2; i <= 32; i++)
			writeFile(pathJoin("testdir/files", Stringizer() + i));
	}

	{
		CAGE_TESTCASE("list directory");
		Holder<DirectoryList> fs = newDirectoryList("testdir/files");
		CAGE_TEST(fs);
		std::set<String> mp;
		while (fs->valid())
		{
			mp.insert(fs->name());
			fs->next();
		}
		CAGE_TEST(mp.size() == 32);
		for (uint32 i = 1; i <= 32; i++)
			CAGE_TEST(mp.count(Stringizer() + i));
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
			String sa({ &a, &a + 1 });
			for (char b = 'a'; b < 'e'; b++)
			{
				String sb({ &b, &b + 1 });
				for (char c = 'a'; c < a; c++)
				{
					String sc({ &c, &c + 1 });
					writeFile(pathJoin(pathJoin("testdir/files", sa), pathJoin(sb, sc)));
				}
				writeFile(pathJoin(pathJoin("testdir/files", sa), pathJoin(sb, "e")));
			}
			for (char b = 'e'; b < 'h'; b++)
			{
				String sb({ &b, &b + 1 });
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

		const String data = "ratata://omega.alt.com/blah/keee/jojo.armagedon";

		{
			Holder<File> f = writeFile("testdir/files/lines");
			String s = data;
			while (!s.empty())
				f->writeLine(split(s, "/"));
			f->close();
		}

		{
			Holder<File> f = readFile("testdir/files/lines");
			String s;
			uint32 cnt = 0;
			for (String line; f->readLine(line);)
			{
				s += line + "/";
				cnt++;
			}
			CAGE_TEST(s == data + "/");
			CAGE_TEST(cnt == 6);
		}
	}

	{
		CAGE_TESTCASE("move file");

		{
			CAGE_TESTCASE("simple move");
			const String source = "testdir/files/1";
			const String dest = "testdir/moved/1";
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
			Holder<File> f = newFileBuffer(Holder<MemoryBuffer>(&data, nullptr), FileMode(true, false));
			readInMemoryFile(f);
		}
		{
			auto pr = PointerRange<char>(data);
			Holder<File> f = newFileBuffer(Holder<PointerRange<char>>(&pr, nullptr)); // not good practice - the pointer to pr is dangerous
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

	{
		CAGE_TESTCASE("invalid file paths");
		String invalidPath = "invalid-path";
		invalidPath[3] = 0;
		CAGE_TEST(pathType(invalidPath) == PathTypeFlags::Invalid);
		CAGE_TEST(!pathIsFile(invalidPath));
		CAGE_TEST_THROWN(pathRemove(invalidPath));
		CAGE_TEST_THROWN(pathMove(invalidPath, "valid-path"));
		CAGE_TEST_THROWN(pathMove("valid-path", invalidPath));
		CAGE_TEST_THROWN(readFile(invalidPath));
		CAGE_TEST_THROWN(writeFile(invalidPath));
		CAGE_TEST_THROWN(pathLastChange(invalidPath));
		CAGE_TEST_THROWN(pathCreateDirectories(invalidPath));
		CAGE_TEST_THROWN(pathCreateArchive(invalidPath));
		CAGE_TEST_THROWN(newDirectoryList(invalidPath));
		CAGE_TEST_THROWN(pathSearchTowardsRoot(invalidPath));
	}

	{
		CAGE_TESTCASE("sanitize file path");
		const String d = "testdir/dangerous/abc'\"^°`_-:?!%;#~(){}[]<>def.bin";
		CAGE_TEST_THROWN(writeFile(d));
		const String s = pathReplaceInvalidCharacters(d, "_", true);
		CAGE_LOG(SeverityEnum::Info, "tests", Stringizer() + "sanitized path: '" + s + "'");
		CAGE_TEST(writeFile(s));
	}

	{
		CAGE_TESTCASE("filesystem watcher");
		Holder<FilesystemWatcher> w = newFilesystemWatcher();
		pathCreateDirectories("testdir/watch/dir");
		w->registerPath("testdir/watch");
		CAGE_TEST(w->waitForChange(0) == "");
		writeFile("testdir/watch/1");
		CAGE_TEST(w->waitForChange(0) == pathToAbs("testdir/watch/1"));
		CAGE_TEST(w->waitForChange(0) == "");
		writeFile("testdir/watch/dir/2");
		CAGE_TEST(w->waitForChange(0) == pathToAbs("testdir/watch/dir/2"));
		CAGE_TEST(w->waitForChange(0) == "");
	}
}
