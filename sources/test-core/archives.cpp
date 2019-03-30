#include <set>

#include "main.h"
#include <cage-core/filesystem.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/math.h>

namespace
{
	static const string directories[2] = {
		"testdir/files",
		"testdir/archive.zip"
	};

	void testWriteFile(const string &name, const memoryBuffer &data)
	{
		for (const string &dir : directories)
		{
			holder<fileClass> f = newFile(pathJoin(dir, name), fileMode(false, true));
			f->writeBuffer(data);
			f->close();
		}
	}

	void testReadFile2(const string &name, memoryBuffer &a, memoryBuffer &b)
	{
		memoryBuffer *p[2] = { &a, &b };
		uint32 i = 0;
		for (const string &dir : directories)
		{
			holder<fileClass> f = newFile(pathJoin(dir, name), fileMode(true, false));
			*p[i++] = f->readBuffer(f->size());
			f->close();
		}
	}

	memoryBuffer testReadFile(const string &name)
	{
		memoryBuffer a, b;
		testReadFile2(name, a, b);
		CAGE_TEST(a.size() == b.size());
		CAGE_TEST(detail::memcmp(a.data(), b.data(), a.size()) == 0);
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
			holder<directoryListClass> f = newDirectoryList(pathJoin(dir, name));
			while (f->valid())
			{
				p[i]->insert(std::make_pair<string, bool>(f->name(), f->isDirectory()));
				f->next();
			}
			i++;
		}
	}

	pathTypeFlags testPathType(const string &name)
	{
		pathTypeFlags a, b;
		{
			pathTypeFlags *p[2] = { &a, &b };
			uint32 i = 0;
			for (const string &dir : directories)
				*p[i++] = pathType(pathJoin(dir, name));
		}
		{
			pathTypeFlags m = pathTypeFlags::File | pathTypeFlags::Directory;
			CAGE_TEST((a & m) == (b & m));
		}
		CAGE_TEST((a & pathTypeFlags::InsideArchive) == pathTypeFlags::None);
		CAGE_TEST((b & pathTypeFlags::InsideArchive) == pathTypeFlags::InsideArchive);
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
		CAGE_TEST(pathType(directories[1]) == (pathTypeFlags::File | pathTypeFlags::Archive));
		testListDirectory("");
	}

	{
		CAGE_TESTCASE("file with text");
		memoryBuffer b;
		serializer s(b);
		s << string("i have no idea what am i doing");
		testWriteFile("text", b);
		testReadFile("text");
		testListDirectory("");
	}

	memoryBuffer data1, data2, data3;
	{ // generate some random data
		memoryBuffer *data[3] = { &data1, &data2, &data3 };
		for (memoryBuffer *d : data)
		{
			serializer ser(*d);
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
				name += randomRange(0, 9);
				if (randomChance() < 0.1)
					name += "/";
			}
			testWriteFile(name, data1);
		}
		testListRecursive();
	}

	// todo interleaved reading/writing from/to multiple files
	// todo seeking in files
	// todo removing files
	// todo moving files
	// todo concurrent reading/writing from/to multiple files
	// todo archives inside archives (most likely wont implement)
}
