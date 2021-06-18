#include "main.h"
#include <cage-core/files.h>
#include <cage-core/lineReader.h>

void testLineReader()
{
	CAGE_TESTCASE("line reader");

	const PointerRange<const char> data = "abc\ndef\r\nghi\n\nlast";
	CAGE_ASSERT(data.size() == 18); // sanity check

	{
		CAGE_TESTCASE("line reader from memory (pointer ranges)");
		Holder<LineReader> lr = newLineReader(data);
		PointerRange<const char> l;
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(string(l) == "abc");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(string(l) == "def");
		CAGE_TEST(l.begin() == data.begin() + 4);
		CAGE_TEST(l.end() == data.begin() + 7);
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(string(l) == "ghi");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(string(l) == "");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(string(l) == "last");
		CAGE_TEST(!lr->readLine(l));
	}

	{
		CAGE_TESTCASE("line reader from memory (strings)");
		Holder<LineReader> lr = newLineReader(data);
		string l;
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "abc");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "def");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "ghi");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "last");
		CAGE_TEST(!lr->readLine(l));
	}

	{
		CAGE_TESTCASE("write file");
		writeFile("testdir/lineReaderTest")->write(data);
	}

	{
		CAGE_TESTCASE("line reading from file");
		Holder<File> lr = readFile("testdir/lineReaderTest");
		string l;
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "abc");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "def");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "ghi");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "");
		CAGE_TEST(lr->readLine(l));
		CAGE_TEST(l == "last");
		CAGE_TEST(!lr->readLine(l));
	}

	{
		// reading a file in text mode has been disabled
		CAGE_TESTCASE("line reading from text file");
		FileMode fm(true, false);
		fm.textual = true;
		CAGE_TEST_ASSERTED(newFile("testdir/lineReaderTest", fm));
	}
}
