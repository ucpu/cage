#include "main.h"
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>

namespace
{
}

void testArchivesRecursion()
{
	CAGE_TESTCASE("archives recursion");

	pathRemove("testdir");

	{
		CAGE_TESTCASE("create archive inside archive inside archive ...");
		string p = "testdir";
		for (uint32 i = 0; i < 5; i++)
		{
			p = pathJoin(p, stringizer() + i + ".zip");
			pathCreateArchive(p);
		}
		CAGE_TEST(none(pathType("testdir") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir") & PathTypeFlags::InsideArchive));
		CAGE_TEST(any(pathType("testdir") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip") & PathTypeFlags::InsideArchive));
		CAGE_TEST(none(pathType("testdir/0.zip") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip") & PathTypeFlags::Archive));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip") & PathTypeFlags::InsideArchive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip") & PathTypeFlags::Archive));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip") & PathTypeFlags::InsideArchive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip") & PathTypeFlags::Directory));
	}

	{
		CAGE_TESTCASE("create file inside each archive");
		string p = "testdir";
		for (uint32 i = 0; i < 5; i++)
		{
			p = pathJoin(p, stringizer() + i + ".zip");
			Holder<File> f = writeFile(pathJoin(p, "welcome"));
			f->writeLine("hello");
			f->close();
		}
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::File));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::InsideArchive));
	}

	{
		CAGE_TESTCASE("read file inside each archive");
		string p = "testdir";
		for (uint32 i = 0; i < 5; i++)
		{
			p = pathJoin(p, stringizer() + i + ".zip");
			Holder<File> f = readFile(pathJoin(p, "welcome"));
			string l;
			CAGE_TEST(f->readLine(l));
			CAGE_TEST(l == "hello");
			f->close();
		}
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Archive));
		CAGE_TEST(none(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::Directory));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::File));
		CAGE_TEST(any(pathType("testdir/0.zip/1.zip/2.zip/welcome") & PathTypeFlags::InsideArchive));
	}

	{
		CAGE_TESTCASE("opening an archive (inside archive) as regular file");
		{
			CAGE_TESTCASE("with open archive");
			string p = "testdir";
			for (uint32 i = 0; i < 5; i++)
			{
				p = pathJoin(p, stringizer() + i + ".zip");
				Holder<DirectoryList> list = newDirectoryList(p); // ensure the archive is open
				CAGE_TEST(list->valid()); // sanity check that there is at least one file in the archive
				CAGE_TEST_THROWN(readFile(p));
			}
		}
		{
			CAGE_TESTCASE("with closed archive");
			string p = "testdir";
			for (uint32 i = 0; i < 5; i++)
			{
				p = pathJoin(p, stringizer() + i + ".zip");
				CAGE_TEST_THROWN(readFile(p));
			}
		}
	}
}
