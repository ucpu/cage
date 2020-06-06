#include "main.h"
#include <cage-core/files.h>
#include <cage-core/lineReader.h>

void testLineReader()
{
	CAGE_TESTCASE("line reader");

	const string data("abc\ndef\r\nghi\n\nlast");
	{
		writeFile("testdir/lineReaderTest")->write({ data.c_str(), data.c_str() + data.length() });
	}

	{
		CAGE_TESTCASE("line reader from memory");
		Holder<LineReader> lr = newLineReader({ data.c_str(), data.c_str() + data.length() });
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
		CAGE_TESTCASE("line reading from file (binary)");
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
		CAGE_TESTCASE("line reading from file (textual)");
		FileMode fm(true, false);
		fm.textual = true;
		CAGE_TEST_ASSERTED(newFile("testdir/lineReaderTest", fm));
	}
}
