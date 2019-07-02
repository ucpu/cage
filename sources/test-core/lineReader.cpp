#include "main.h"
#include <cage-core/files.h>
#include <cage-core/lineReader.h>

void testLineReader()
{
	CAGE_TESTCASE("line reader");

	const string data("abc\ndef\r\nghi\n\nlast");
	{
		newFile("testdir/lineReaderTest", fileMode(false, true))->write(data.c_str(), data.length());
	}

	{
		CAGE_TESTCASE("line reader from memory");
		holder<lineReader> lr = newLineReader(data.c_str(), data.length());
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
		holder<file> lr = newFile("testdir/lineReaderTest", fileMode(true, false));
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
		fileMode fm(true, false);
		fm.textual = true;
		CAGE_TEST_ASSERTED(newFile("testdir/lineReaderTest", fm));
	}
}
