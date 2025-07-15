#include <cstdlib>
#include <set>

#include "main.h"

#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

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
		const auto list = pathListDirectory("testdir/files");
		CAGE_TEST(list);
		std::set<String> mp;
		for (const String &n : list)
			mp.insert(pathExtractFilename(n));
		CAGE_TEST(mp.size() == 32);
		for (uint32 i = 1; i <= 32; i++)
			CAGE_TEST(mp.count(Stringizer() + i));
		CAGE_TEST(pathType("testdir/files/1") == PathTypeFlags::File);
		CAGE_TEST_THROWN(pathListDirectory("testdir/files/1"));
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
		const auto list = pathListDirectory("testdir/files/d/b");
		CAGE_TEST(list->size() == 4);
	}

	{
		CAGE_TESTCASE("non-existing file");
		CAGE_TEST(pathType("testdir/files/non-existing-file") == PathTypeFlags::NotFound);
		CAGE_TEST(pathType("testdir/files/non-existing-file") == PathTypeFlags::NotFound); // sanity check - the first pathType may not create it
		CAGE_TEST_THROWN(readFile("testdir/files/non-existing-file"));
		CAGE_TEST_THROWN(pathListDirectory("testdir/files/non-existing-file"));
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
		CAGE_TESTCASE("move (rename)");

		{
			CAGE_TESTCASE("simple move");
			static constexpr const String source = "testdir/files/1";
			static constexpr const String dest = "testdir/moved/1";
			CAGE_TEST(pathIsFile(source));
			CAGE_TEST(!pathIsFile(dest));
			pathMove(source, dest);
			CAGE_TEST(!pathIsFile(source));
			CAGE_TEST(pathIsFile(dest));
		}

		{
			CAGE_TESTCASE("rename file");
			static constexpr const String source = "testdir/files/2";
			static constexpr const String dest = "testdir/files/renamed";
			CAGE_TEST(pathIsFile(source));
			CAGE_TEST(!pathIsFile(dest));
			pathMove(source, dest);
			CAGE_TEST(!pathIsFile(source));
			CAGE_TEST(pathIsFile(dest));
		}

		{
			CAGE_TESTCASE("rename folder");
			writeFile("testdir/rename1/1")->writeLine("haha");
			writeFile("testdir/rename1/2")->writeLine("haha");
			CAGE_TEST(pathIsDirectory("testdir/rename1"));
			CAGE_TEST(!pathIsDirectory("testdir/rename2"));
			pathMove("testdir/rename1", "testdir/rename2");
			CAGE_TEST(!pathIsDirectory("testdir/rename1"));
			CAGE_TEST(pathIsDirectory("testdir/rename2"));
		}

		{
			CAGE_TESTCASE("move to non-existing directory");
			pathMove("testdir/moved/1", "testdir/moved/non-exist/1");
		}

		{
			CAGE_TESTCASE("move non-existing file");
			CAGE_TEST_THROWN(pathMove("testdir/moved/non-existing-file", "testdir/moved/1"));
		}

		{
			CAGE_TESTCASE("move folder");
			writeFile("testdir/moved/2")->writeLine("haha");
			writeFile("testdir/moved/3")->writeLine("haha");
			CAGE_TEST(pathListDirectory("testdir/moved").size() == 3);
			pathMove("testdir/moved", "testdir/moved2");
			CAGE_TEST(!pathIsDirectory("testdir/moved"));
			CAGE_TEST(pathIsDirectory("testdir/moved2"));
			CAGE_TEST(pathListDirectory("testdir/moved2").size() == 3);
		}

		{
			CAGE_TESTCASE("move to itself");
			writeFile("testdir/moving")->writeLine("haha");
			pathMove("testdir/moving", "testdir/moving");
			CAGE_TEST(pathIsFile("testdir/moving"));
			CAGE_TEST(readFile("testdir/moving")->readLine() == "haha");
		}

		{
			CAGE_TESTCASE("merge move folder");
			writeFile("testdir/ttt/5")->writeLine("haha");
			writeFile("testdir/ttt/6")->writeLine("haha");
			pathMove("testdir/ttt", "testdir/moved2");
			CAGE_TEST(!pathIsDirectory("testdir/ttt"));
			CAGE_TEST(pathIsDirectory("testdir/moved2"));
			CAGE_TEST(pathListDirectory("testdir/moved2").size() == 5);
		}

		{
			CAGE_TESTCASE("move inside itself");
			CAGE_TEST_THROWN(pathMove("testdir", "testdir/moved2"));
		}
	}

	{
		CAGE_TESTCASE("copy");

		{
			CAGE_TESTCASE("simple copy");
			static constexpr const String source = "testdir/files/3";
			static constexpr const String dest = "testdir/copied/3";
			writeFile(source)->writeLine("haha");
			CAGE_TEST(pathIsFile(source));
			CAGE_TEST(!pathIsFile(dest));
			pathCopy(source, dest);
			CAGE_TEST(pathIsFile(source));
			CAGE_TEST(pathIsFile(dest));
		}

		{
			CAGE_TESTCASE("copy to non-existing directory");
			writeFile("testdir/files/4")->writeLine("haha");
			pathCopy("testdir/files/4", "testdir/copied/non-exist2/4");
		}

		{
			CAGE_TESTCASE("copy non-existing file");
			CAGE_TEST_THROWN(pathCopy("testdir/copied/non-existing-file2", "testdir/copied/2"));
		}

		{
			CAGE_TESTCASE("copy folder");
			writeFile("testdir/copied/2")->writeLine("haha");
			writeFile("testdir/copied/3")->writeLine("haha");
			CAGE_TEST(pathListDirectory("testdir/copied").size() == 3);
			pathCopy("testdir/copied", "testdir/copied2");
			CAGE_TEST(pathIsDirectory("testdir/copied"));
			CAGE_TEST(pathIsDirectory("testdir/copied2"));
			CAGE_TEST(pathListDirectory("testdir/copied").size() == 3);
			CAGE_TEST(pathListDirectory("testdir/copied2").size() == 3);
		}

		{
			CAGE_TESTCASE("copy to itself");
			pathCopy("testdir/copied2", "testdir/copied2");
			CAGE_TEST(pathIsDirectory("testdir/copied2"));
			CAGE_TEST(pathListDirectory("testdir/copied2").size() == 3);
		}

		{
			CAGE_TESTCASE("merge copy folder");
			writeFile("testdir/ttt/7")->writeLine("haha");
			writeFile("testdir/ttt/8")->writeLine("haha");
			pathCopy("testdir/ttt", "testdir/copied2");
			CAGE_TEST(pathIsDirectory("testdir/ttt"));
			CAGE_TEST(pathListDirectory("testdir/ttt").size() == 2);
			CAGE_TEST(pathIsDirectory("testdir/copied2"));
			CAGE_TEST(pathListDirectory("testdir/copied2").size() == 5);
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
		CAGE_TEST_THROWN(pathCreateArchiveZip(invalidPath));
		CAGE_TEST_THROWN(pathCreateArchiveCarch(invalidPath));
		CAGE_TEST_THROWN(pathListDirectory(invalidPath));
		CAGE_TEST_THROWN(pathSearchTowardsRoot(invalidPath));
	}

	{
		CAGE_TESTCASE("sanitize file path");
		const String d = "testdir/dangerous/abc'\"^°`_-:?!%;#~(){}[]<>def\7gжяhi.bin";
		CAGE_TEST_THROWN(writeFile(d));
		const String s = pathReplaceInvalidCharacters(d, "_", true);
		CAGE_LOG(SeverityEnum::Info, "tests", Stringizer() + "sanitized path: " + s);
		CAGE_TEST(writeFile(s));
	}

	{
		CAGE_TESTCASE("unicode names");

		{
			CAGE_TESTCASE("create files");
			// http://www.madore.org/~david/misc/unitest/
			writeFile("testdir/unicode/1-Пооживлённымберегам");
			writeFile("testdir/unicode/2-Ἰοὺἰού·τὰπάντʼἂνἐξήκοισαφῆ");
			writeFile("testdir/unicode/3-पशुपतिरपितान्यहानिकृच्छ्राद्");
			writeFile("testdir/unicode/4-子曰「學而時習之不亦說乎");
			writeFile("testdir/unicode/5-ஸ்றீனிவாஸராமானுஜன்ஐயங்கார்");
			writeFile("testdir/unicode/6-بِسْمِٱللّٰهِٱلرَّحْمـَبنِٱلرَّحِيمِ");
			writeFile("testdir/unicode/7-По/оживлённым/берегам");
		}

		{
			CAGE_TESTCASE("test files");
			CAGE_TEST(pathType("testdir/unicode/1-Пооживлённымберегам") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/2-Ἰοὺἰού·τὰπάντʼἂνἐξήκοισαφῆ") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/3-पशुपतिरपितान्यहानिकृच्छ्राद्") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/4-子曰「學而時習之不亦說乎") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/5-ஸ்றீனிவாஸராமானுஜன்ஐயங்கார்") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/6-بِسْمِٱللّٰهِٱلرَّحْمـَبنِٱلرَّحِيمِ") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/7-По/оживлённым/берегам") == PathTypeFlags::File);
			CAGE_TEST(pathType("testdir/unicode/7-По/оживлённым") == PathTypeFlags::Directory);
			CAGE_TEST(pathType("testdir/unicode/42-بِسْمِٱللّٰهِٱلرَّحْمـَبنِٱلرَّحِيمِ") == PathTypeFlags::NotFound);
		}

		{
			CAGE_TESTCASE("list files");
			const auto list = pathListDirectory("testdir/unicode");
			CAGE_TEST(list);
			for (const String &n : list)
				CAGE_LOG(SeverityEnum::Info, "unicode-test", n);
		}

		{
			CAGE_TESTCASE("move files");
			pathMove("testdir/unicode/1-Пооживлённымберегам", "testdir/unicode/1_Пооживлённымберегам");
			pathMove("testdir/unicode/2-Ἰοὺἰού·τὰπάντʼἂνἐξήκοισαφῆ", "testdir/unicode/2_Ἰοὺἰού·τὰπάντʼἂνἐξήκοισαφῆ");
			pathMove("testdir/unicode/3-पशुपतिरपितान्यहानिकृच्छ्राद्", "testdir/unicode/3_पशुपतिरपितान्यहानिकृच्छ्राद्");
			pathMove("testdir/unicode/4-子曰「學而時習之不亦說乎", "testdir/unicode/4_子曰「學而時習之不亦說乎");
			pathMove("testdir/unicode/5-ஸ்றீனிவாஸராமானுஜன்ஐயங்கார்", "testdir/unicode/5_ஸ்றீனிவாஸராமானுஜன்ஐயங்கார்");
			pathMove("testdir/unicode/6-بِسْمِٱللّٰهِٱلرَّحْمـَبنِٱلرَّحِيمِ", "testdir/unicode/6_بِسْمِٱللّٰهِٱلرَّحْمـَبنِٱلرَّحِيمِ");
			pathMove("testdir/unicode/7-По/оживлённым/берегам", "testdir/unicode/7_По/оживлённым/берегам");
		}

		{
			CAGE_TESTCASE("remove files");
			pathRemove("testdir/unicode/4_子曰「學而時習之不亦說乎");
			pathRemove("testdir/unicode/7_По");
		}
	}

	{
		CAGE_TESTCASE("paths find sequence");

		writeFile("testdir/sequence/001.txt");
		writeFile("testdir/sequence/002.txt");
		writeFile("testdir/sequence/003.txt");
		writeFile("testdir/sequence/004.txt");
		writeFile("testdir/sequence/005.txt");
		writeFile("testdir/sequence/006.txt");
		writeFile("testdir/sequence/007.txt");
		writeFile("testdir/sequence/008.txt");
		writeFile("testdir/sequence/009.txt");
		writeFile("testdir/sequence/010.txt");
		writeFile("testdir/sequence/011.txt");
		writeFile("testdir/sequence/012.txt");
		writeFile("testdir/sequence/013.txt");

		{
			const auto res = pathSearchSequence("testdir/sequence/$$$.txt");
			CAGE_TEST(res.size() == 13);
			CAGE_TEST(res[0] == "testdir/sequence/001.txt");
			CAGE_TEST(res[12] == "testdir/sequence/013.txt");
		}

		{
			const auto res = pathSearchSequence("testdir/sequence/$.txt");
			CAGE_TEST(res.size() == 0);
		}

		writeFile("testdir/sequence/000.txt");

		{
			const auto res = pathSearchSequence("testdir/sequence/$$$.txt");
			CAGE_TEST(res.size() == 14);
			CAGE_TEST(res[0] == "testdir/sequence/000.txt");
			CAGE_TEST(res[13] == "testdir/sequence/013.txt");
		}

		pathRemove("testdir/sequence");

		writeFile("testdir/sequence/1.txt");
		writeFile("testdir/sequence/2.txt");
		writeFile("testdir/sequence/3.txt");
		writeFile("testdir/sequence/4.txt");
		writeFile("testdir/sequence/5.txt");
		writeFile("testdir/sequence/6.txt");
		writeFile("testdir/sequence/7.txt");
		writeFile("testdir/sequence/8.txt");
		writeFile("testdir/sequence/9.txt");
		writeFile("testdir/sequence/10.txt");
		writeFile("testdir/sequence/11.txt");
		writeFile("testdir/sequence/12.txt");
		writeFile("testdir/sequence/13.txt");

		{
			const auto res = pathSearchSequence("testdir/sequence/$.txt");
			CAGE_TEST(res.size() == 13);
			CAGE_TEST(res[0] == "testdir/sequence/1.txt");
			CAGE_TEST(res[12] == "testdir/sequence/13.txt");
		}

		{
			const auto res = pathSearchSequence("testdir/sequence/$$$.txt");
			CAGE_TEST(res.size() == 0);
		}

		writeFile("testdir/sequence/0.txt");

		{
			const auto res = pathSearchSequence("testdir/sequence/$.txt");
			CAGE_TEST(res.size() == 14);
			CAGE_TEST(res[0] == "testdir/sequence/0.txt");
			CAGE_TEST(res[13] == "testdir/sequence/13.txt");
		}

		{
			const auto res = pathSearchSequence("testdir/sequence/13.txt");
			CAGE_TEST(res.size() == 1);
			CAGE_TEST(res[0] == "testdir/sequence/13.txt");
		}

		{
			const auto res = pathSearchSequence("testdir/sequence/42.txt");
			CAGE_TEST(res.size() == 0);
		}
	}

	{
		CAGE_TESTCASE("filesystem watcher");
#ifdef CAGE_SYSTEM_MAC
		CAGE_LOG(SeverityEnum::Warning, "tests", "skipping the test - macos");
#else
		if (isPattern(pathWorkingDir(), "/mnt/", "", ""))
		{
			CAGE_LOG(SeverityEnum::Warning, "tests", "skipping the test - we are possibly running on a filesystem that does not support watching");
		}
		else
		{
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
#endif
	}
}
