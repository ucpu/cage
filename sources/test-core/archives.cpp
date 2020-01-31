#include "main.h"
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/math.h>

#include <set>

namespace
{
	static const string directories[2] = {
		"testdir/files",
		"testdir/archive.zip"
	};

	void testWriteFile(const string &name, const MemoryBuffer &data)
	{
		for (const string &dir : directories)
		{
			Holder<File> f = newFile(pathJoin(dir, name), FileMode(false, true));
			f->writeBuffer(data);
			f->close();
		}
	}

	void testReadFile2(const string &name, MemoryBuffer &a, MemoryBuffer &b)
	{
		MemoryBuffer *p[2] = { &a, &b };
		uint32 i = 0;
		for (const string &dir : directories)
		{
			Holder<File> f = newFile(pathJoin(dir, name), FileMode(true, false));
			*p[i++] = f->readBuffer(f->size());
			f->close();
		}
	}

	void testBuffers(const MemoryBuffer &a, const MemoryBuffer &b)
	{
		CAGE_TEST(a.size() == b.size());
		CAGE_TEST(detail::memcmp(a.data(), b.data(), a.size()) == 0);
	}

	MemoryBuffer testReadFile(const string &name)
	{
		MemoryBuffer a, b;
		testReadFile2(name, a, b);
		testBuffers(a, b);
		return a;
	}

	void testCreateDirectories(const string &name)
	{
		for (const string &dir : directories)
			pathCreateDirectories(pathJoin(dir, name));
	}

	typedef std::set<std::pair<string, bool>> listing;

	void testListDirectory2(const string &name, listing &a, listing &b)
	{
		listing *p[2] = { &a, &b };
		uint32 i = 0;
		for (const string &dir : directories)
		{
			Holder<DirectoryList> f = newDirectoryList(pathJoin(dir, name));
			while (f->valid())
			{
				p[i]->insert(std::make_pair<string, bool>(f->name(), f->isDirectory()));
				f->next();
			}
			i++;
		}
	}

	PathTypeFlags testPathType(const string &name)
	{
		PathTypeFlags a, b;
		{
			PathTypeFlags *p[2] = { &a, &b };
			uint32 i = 0;
			for (const string &dir : directories)
				*p[i++] = pathType(pathJoin(dir, name));
		}
		{
			PathTypeFlags m = PathTypeFlags::File | PathTypeFlags::Directory;
			CAGE_TEST((a & m) == (b & m));
		}
		CAGE_TEST((a & PathTypeFlags::InsideArchive) == PathTypeFlags::None);
		CAGE_TEST((b & PathTypeFlags::InsideArchive) == PathTypeFlags::InsideArchive);
		return b;
	}

	listing testListDirectory(const string &name)
	{
		listing a, b;
		testListDirectory2(name, a, b);
		CAGE_TEST(a.size() == b.size());
		CAGE_TEST(a == b);
		for (const auto &n : a)
			testPathType(n.first);
		return a;
	}

	void testListRecursive(const string &name = "")
	{
		listing l = testListDirectory(name);
		for (const auto &i : l)
		{
			if (i.second)
			{
				string p = pathJoin(name, i.first);
				testListRecursive(p);
			}
		}
	}
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

	MemoryBuffer data1, data2, data3;
	{ // generate some random data
		MemoryBuffer *data[3] = { &data1, &data2, &data3 };
		for (MemoryBuffer *d : data)
		{
			Serializer ser(*d);
			uint32 cnt = randomRange(10, 100);
			for (uint32 i = 0; i < cnt; i++)
				ser << randomRange(-1.0, 1.0);
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
			string name = "rng";
			for (uint32 j = 0; j < 10; j++)
			{
				name += string(randomRange(0, 9));
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
				newFile(pathJoin(directories[0], "multi/1"), FileMode(false, true)),
				newFile(pathJoin(directories[1], "multi/1"), FileMode(false, true)),
				newFile(pathJoin(directories[0], "multi/2"), FileMode(false, true)),
				newFile(pathJoin(directories[1], "multi/2"), FileMode(false, true)),
				newFile(pathJoin(directories[0], "multi/3"), FileMode(false, true)),
				newFile(pathJoin(directories[1], "multi/3"), FileMode(false, true))
			};
			for (auto &f : ws)
				f->writeBuffer(data1);
			for (auto &f : ws)
				f->writeBuffer(data2);
			for (auto &f : ws)
				f->writeBuffer(data3);
		}
		testReadFile("multi/1");
		testListRecursive();
	}

	{
		CAGE_TESTCASE("multiple read files at once");
		Holder<File> rs[] = {
			newFile(pathJoin(directories[0], "multi/1"), FileMode(true, false)),
			newFile(pathJoin(directories[1], "multi/1"), FileMode(true, false)),
			newFile(pathJoin(directories[0], "multi/2"), FileMode(true, false)),
			newFile(pathJoin(directories[1], "multi/2"), FileMode(true, false)),
			newFile(pathJoin(directories[0], "multi/3"), FileMode(true, false)),
			newFile(pathJoin(directories[1], "multi/3"), FileMode(true, false))
		};
		for (auto &f : rs)
			f->readBuffer(data3.size());
		for (auto &f : rs)
			f->readBuffer(data2.size());
		for (auto &f : rs)
			f->readBuffer(data1.size());
	}

	{
		CAGE_TESTCASE("file seek & tell");
		string path = pathJoin(directories[1], "multi/1");
		PathTypeFlags type = pathType(path);
		CAGE_TEST((type & PathTypeFlags::File) == PathTypeFlags::File);
		CAGE_TEST((type & PathTypeFlags::Directory) == PathTypeFlags::None);
		CAGE_TEST((type & PathTypeFlags::InsideArchive) == PathTypeFlags::InsideArchive);
		CAGE_TEST((type & PathTypeFlags::Archive) == PathTypeFlags::None);
		Holder<File> f = newFile(path, FileMode(true, false));
		f->seek(data1.size() + data2.size());
		MemoryBuffer b3 = f->readBuffer(data3.size());
		CAGE_TEST(f->tell() == f->size());
		f->seek(data1.size());
		CAGE_TEST(f->tell() == data1.size());
		MemoryBuffer b2 = f->readBuffer(data2.size());
		CAGE_TEST(f->tell() == data1.size() + data2.size());
		f->seek(0);
		CAGE_TEST(f->tell() == 0);
		MemoryBuffer b1 = f->readBuffer(data1.size());
		CAGE_TEST(f->tell() == data1.size());
		f->close();
		testBuffers(b1, data1);
		testBuffers(b2, data2);
		testBuffers(b3, data3);
	}

	{
		CAGE_TESTCASE("remove a file");
		{
			Holder<DirectoryList> list = newDirectoryList(directories[1]); // keep the archive open
			for (const string &dir : directories)
				pathRemove(pathJoin(dir, "multi/2"));
			testListRecursive(); // listing must work even while the changes to the archive are not yet written out
			CAGE_TEST((pathType(pathJoin(directories[1], "multi/2")) & PathTypeFlags::NotFound) == PathTypeFlags::NotFound);
		}
		testListRecursive(); // and after they are written too
	}

	{
		CAGE_TESTCASE("move a file");
		{
			CAGE_TESTCASE("inside same archive");
			for (const string &dir : directories)
				pathMove(pathJoin(dir, "multi/3"), pathJoin(dir, "multi/4"));
			testListRecursive();
		}
		{
			CAGE_TESTCASE("from real filesystem to the archive");
			for (const string &dir : directories)
			{
				{
					Holder<File> f = newFile("testdir/tmp", FileMode(false, true));
					f->writeBuffer(data2);
				}
				pathMove("testdir/tmp", pathJoin(dir, "moved"));
			}
			testReadFile("moved");
			testListRecursive();
		}
		{
			CAGE_TESTCASE("from the archive to real filesystem");
			for (const string &dir : directories)
				pathMove(pathJoin(dir, "multi/1"), "testdir/moved");
			testListRecursive();
		}
	}

	// todo lastChange
	// todo concurrent reading/writing from/to multiple (different) files
	// todo archives inside archives
}
