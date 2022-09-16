#include "main.h"

#include <cage-core/files.h>

namespace
{
	void testPathSimple(const String &input, const String &simple, const bool absolute, const bool valid = true)
	{
		CAGE_TESTCASE(input);
		CAGE_TEST(pathIsValid(input) == valid);
		if (valid)
		{
			CAGE_TEST(pathIsValid(simple)); // sanity check
			CAGE_TEST(pathSimplify(simple) == simple); // sanity check
			CAGE_TEST(pathSimplify(input) == simple);
			CAGE_TEST(pathSimplify(pathSimplify(input)) == simple);
			CAGE_TEST(pathIsAbs(simple) == absolute); // sanity check
			CAGE_TEST(pathIsAbs(input) == absolute);
			if (absolute)
			{
				CAGE_TEST_THROWN(pathJoin("haha", input));
			}
			else
			{
				CAGE_TEST(pathToRel(input) == simple);
			}
			CAGE_TEST(pathJoin(input, "haha") == pathJoin(simple, "haha"));
		}
		else
		{
			CAGE_TEST_THROWN(pathSimplify(input));
			CAGE_TEST_THROWN(pathIsAbs(input));
			CAGE_TEST_THROWN(pathExtractDrive(input));
			CAGE_TEST_THROWN(pathExtractDirectory(input));
			CAGE_TEST_THROWN(pathExtractDirectoryNoDrive(input));
			CAGE_TEST_THROWN(pathExtractFilename(input));
			CAGE_TEST_THROWN(pathExtractFilenameNoExtension(input));
			CAGE_TEST_THROWN(pathExtractExtension(input));
			CAGE_TEST_THROWN(pathToAbs(input));
			CAGE_TEST_THROWN(pathToRel(input));
			CAGE_TEST_THROWN(pathJoin(input, "haha"));
		}
	}

	void testPathDecompositionCommon(const String &input, const String &drive, const String &path, const String &file, const String &extension, const bool absolute)
	{
		CAGE_TESTCASE(input);
		CAGE_TEST(pathIsValid(input)); // sanity check
		CAGE_TEST(drive.empty() || absolute); // sanity check
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		CAGE_TEST(d == drive);
		CAGE_TEST(p == path);
		CAGE_TEST(f == file);
		CAGE_TEST(e == extension);
		CAGE_TEST(pathIsAbs(input) == absolute);
		CAGE_TEST(pathIsAbs(path) == absolute); // sanity check
		CAGE_TEST(pathIsAbs(p) == absolute);
		CAGE_TEST(pathExtractDrive(input) == drive);
		CAGE_TEST(pathExtractDirectory(input) == (drive.empty() ? path : String() + drive + ":" + path));
		CAGE_TEST(pathExtractDirectoryNoDrive(input) == path);
		CAGE_TEST(pathExtractFilename(input) == file + extension);
		CAGE_TEST(pathExtractFilenameNoExtension(input) == file);
		CAGE_TEST(pathExtractExtension(input) == extension);
	}

	void testPathDecomposition(const String &input, const String &drive, const String &path, const String &file, const String &extension, const bool absolute)
	{
		testPathDecompositionCommon(input, drive, path, file, extension, absolute);

		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		CAGE_TEST(pathSimplify(p) == p);
		const String simple = pathSimplify(input);
		{
			const String s = String() + (drive.empty() ? "" : drive + ":") + pathJoin(path, file + extension);
			CAGE_TEST(s == simple);
		}
		if (absolute)
		{
			CAGE_TEST_THROWN(pathJoin("haha", input));
		}
		else
		{
			CAGE_TEST(pathToRel(input) == simple);
		}
		CAGE_TEST(pathJoin(input, "haha") == pathJoin(simple, "haha"));
	}
}

void testPaths()
{
	CAGE_TESTCASE("file paths");

	{
		// some file names are invalid on windows, but are valid on linux
		// this makes testing a lot more complex.
		// for example, it would make sense to allow "windows forbidden characters" inside archives even on windows, since these limitations do not apply to archives
		//   but the functions cannot possibly know where is the path going to be used
		CAGE_TESTCASE("pathReplaceInvalidCharacters");
		constexpr const char *input = "dir/abc'\"^°`_-:?!%;#~(){}[]<>def\7gжяhi.bin";
		const String replaced = pathReplaceInvalidCharacters(input);
#ifdef CAGE_SYSTEM_WINDOWS
		CAGE_TEST(replaced == "dir_abc'_^°`_-__!%;#~(){}[]__def_gжяhi.bin");
#else
		CAGE_TEST(replaced == "dir_abc'\"^°`_-:?!%;#~(){}[]<>def_gжяhi.bin");
#endif // CAGE_SYSTEM_WINDOWS
	}

	{
		CAGE_TESTCASE("basics");
		testPathSimple("", "", false);
		testPathSimple(".", "", false);
		testPathSimple("./", "", false);
		testPathSimple("a", "a", false);
		testPathSimple("a/", "a", false);
		testPathSimple("..", "..", false);
		testPathSimple("a/..", "", false);
		testPathSimple("a/.", "a", false);
		testPathSimple("./a", "a", false);
		testPathSimple(".a", ".a", false);
		testPathSimple("a.", "a.", false);
		testPathSimple("./..", "..", false);
		testPathSimple("../.", "..", false);
		testPathSimple("a/b", "a/b", false);
		testPathSimple("a//b", "a/b", false);
		testPathSimple("a\\b", "a/b", false);
		testPathSimple("a/./b", "a/b", false);
		testPathSimple("b/.a", "b/.a", false);
		testPathSimple("b/a.", "b/a.", false);
		testPathSimple(".a/b", ".a/b", false);
		testPathSimple("a./b", "a./b", false);
		testPathSimple("../..", "../..", false);
		testPathSimple("a/./././b", "a/b", false);
		testPathSimple("hi-Jane", "hi-Jane", false);
		testPathSimple("a/b/c/../d", "a/b/d", false);
		testPathSimple("./file.ext", "file.ext", false);
		testPathSimple("../../../a", "../../../a", false);
		testPathSimple(".////file.ext", "file.ext", false);
		testPathSimple("../file.ext", "../file.ext", false);
		testPathSimple("abc/def/ghi", "abc/def/ghi", false);
		testPathSimple("abc/d.f/ghi", "abc/d.f/ghi", false);
		testPathSimple("abc/d..f/ghi", "abc/d..f/ghi", false);
		testPathSimple("abc/de../ghi", "abc/de../ghi", false);
		testPathSimple("abc/..ef/ghi", "abc/..ef/ghi", false);
		testPathSimple("././././file.ext", "file.ext", false);
		testPathSimple("../../../file.ext", "../../../file.ext", false);

		testPathSimple("/", "/", true);
		testPathSimple("//", "/", true);
		testPathSimple("///", "/", true);
		testPathSimple("/a", "/a", true);
		testPathSimple("k:/", "k:/", true);
		testPathSimple("u:/.", "u:/", true);
		testPathSimple("k://", "k:/", true);
		testPathSimple("k:/a", "k:/a", true);
		testPathSimple("/a/b", "/a/b", true);
		testPathSimple("k://a", "k:/a", true);
		testPathSimple("u:/abc", "u:/abc", true);
		testPathSimple("k:/a/b", "k:/a/b", true);
		testPathSimple("k://a/b", "k:/a/b", true);
		testPathSimple("lol:/omg", "lol:/omg", true);
		testPathSimple("/peter/pan", "/peter/pan", true);
		testPathSimple("/./././file.ext", "/file.ext", true);
		testPathSimple("proto:/path1/path2/file.ext", "proto:/path1/path2/file.ext", true);
		testPathSimple("ratata://omega.alt.com/blah/../jojo.armagedon", "ratata:/omega.alt.com/jojo.armagedon", true);
		testPathSimple("ratata://omega.alt.com/blah/./jojo.armagedon", "ratata:/omega.alt.com/blah/jojo.armagedon", true);
		testPathSimple("ratata:/omega.alt.com/blah/keee/jojo.armagedon", "ratata:/omega.alt.com/blah/keee/jojo.armagedon", true);
		testPathSimple("ratata://omega.alt.com/blah/keee/jojo.armagedon", "ratata:/omega.alt.com/blah/keee/jojo.armagedon", true);

		// invalid
		testPathSimple(":/", "", false, false);
		testPathSimple("/..", "", false, false);
		testPathSimple("/..", "", false, false);
		testPathSimple(":/abc", "", false, false);
		testPathSimple("://abc", "", false, false);
		testPathSimple("/./..", "", false, false);
		testPathSimple("/./..", "", false, false);
		testPathSimple("omg:/..", "", false, false);
		testPathSimple("omg:/..", "", false, false);
		testPathSimple("/a/../..", "", false, false);
		testPathSimple("/a/../..", "", false, false);
		testPathSimple("/../file.ext", "", false, false);
		testPathSimple("omg:/a/../..", "", false, false);
		testPathSimple("omg://a/../..", "", false, false);
		testPathSimple("invalid\7name", "", false, false);
		testPathSimple("invalid\nname", "", false, false);
		testPathSimple("invalid\tname", "", false, false);

		// valid on linux only
#ifdef CAGE_SYSTEM_WINDOWS
		constexpr bool shouldBeValid = false;
#else
		constexpr bool shouldBeValid = true;
#endif // CAGE_SYSTEM_WINDOWS
		testPathSimple(":", ":", false, shouldBeValid);
		testPathSimple(":a", ":a", false, shouldBeValid);
		testPathSimple("u:", "u:", false, shouldBeValid);
		testPathSimple("::", "::", false, shouldBeValid);
		testPathSimple("./:", ":", false, shouldBeValid);
		testPathSimple("u:a", "u:a", false, shouldBeValid);
		testPathSimple(":hhh", ":hhh", false, shouldBeValid);
		testPathSimple("hhh:", "hhh:", false, shouldBeValid);
		testPathSimple("./a:b", "a:b", false, shouldBeValid);
		testPathSimple("/a:/b", "/a:/b", true, shouldBeValid);
		testPathSimple("hhh:..", "hhh:..", false, shouldBeValid);
		testPathSimple("a/b:/c", "a/b:/c", false, shouldBeValid);
		testPathSimple(":./a:b", ":./a:b", false, shouldBeValid);
		testPathSimple("abc?def", "abc?def", false, shouldBeValid);
		testPathSimple("abc*def", "abc*def", false, shouldBeValid);
		testPathSimple("/name1:name2", "/name1:name2", true, shouldBeValid);
		testPathSimple("path/name1:name2", "path/name1:name2", false, shouldBeValid);
		testPathSimple("/path/name1:name2", "/path/name1:name2", true, shouldBeValid);
		testPathSimple("name0:name1:name2", "name0:name1:name2", false, shouldBeValid);
		testPathSimple("/dir/name:with:colons", "/dir/name:with:colons", true, shouldBeValid);
		testPathSimple("/name1:name2/path/file", "/name1:name2/path/file", true, shouldBeValid);
		testPathSimple("hhh:path1/path2/file.ext", "hhh:path1/path2/file.ext", false, shouldBeValid);
		testPathSimple("path/name1:name2/path/file", "path/name1:name2/path/file", false, shouldBeValid);
	}

	{
		CAGE_TESTCASE("pathDecompose");
		testPathDecomposition(".", "", "", "", "", false);
		testPathDecomposition("/", "", "/", "", "", true);
		testPathDecomposition("a/", "", "", "a", "", false);
		testPathDecomposition("..", "", "..", "", "", false);
		testPathDecomposition("a/..", "", "", "", "", false);
		testPathDecomposition("a/.", "", "", "a", "", false);
		testPathDecomposition("../..", "", "../..", "", "", false);
		testPathDecomposition("/a/b/c", "", "/a/b", "c", "", true);
		testPathDecomposition("jup.h", "", "", "jup", ".h", false);
		testPathDecomposition("au.ro.ra", "", "", "au.ro", ".ra", false);
		testPathDecomposition(".htaccess", "", "", "", ".htaccess", false);
		testPathDecomposition("rumprsteak", "", "", "rumprsteak", "", false);
		testPathDecomposition("ratata:/omega", "ratata", "/", "omega", "", true);
		testPathDecomposition("algorithm/astar", "", "algorithm", "astar", "", false);
		testPathDecomposition("/path/../path/file.ext", "", "/path", "file", ".ext", true);
		testPathDecomposition("path/path/path/file.ext", "", "path/path/path", "file", ".ext", false);
		testPathDecomposition("/path/path/path/file.ext", "", "/path/path/path", "file", ".ext", true);
		testPathDecomposition("ratata://omega.alt.com/blah/keee/jojo.armagedon", "ratata", "/omega.alt.com/blah/keee", "jojo", ".armagedon", true);
		testPathDecomposition("ratata:\\\\omega.alt.com\\blah\\keee\\jojo.armagedon", "ratata", "/omega.alt.com/blah/keee", "jojo", ".armagedon", true);
	}

#ifndef CAGE_SYSTEM_WINDOWS
	{
		CAGE_TESTCASE("pathDecompose with linux-only paths");
		testPathDecompositionCommon(":", "", "", ":", "", false);
		testPathDecompositionCommon("::", "", "", "::", "", false);
		testPathDecompositionCommon("/:/", "", "/", ":", "", true);
		testPathDecompositionCommon("./:", "", "", ":", "", false);
		testPathDecompositionCommon(":_:", "", "", ":_:", "", false);
		testPathDecompositionCommon("hhh:", "", "", "hhh:", "", false);
		testPathDecompositionCommon(":name2", "", "", ":name2", "", false);
		testPathDecompositionCommon("/name1:name2", "", "/", "name1:name2", "", true);
		testPathDecompositionCommon("path/name1:name2", "", "path", "name1:name2", "", false);
		testPathDecompositionCommon("/path/name1:name2", "", "/path", "name1:name2", "", true);
		testPathDecompositionCommon("proto:/name1:name2", "proto", "/", "name1:name2", "", true);
		testPathDecompositionCommon("/dir/name:with:colons", "", "/dir", "name:with:colons", "", true);
		testPathDecompositionCommon("/name1:name2/path/file", "", "/name1:name2/path", "file", "", true);
		testPathDecompositionCommon("proto:/path/name1:name2", "proto", "/path", "name1:name2", "", true);
		testPathDecompositionCommon("path/name1:name2/path/file", "", "path/name1:name2/path", "file", "", false);
		testPathDecompositionCommon("hhh:path/path/path/file.ext", "", "hhh:path/path/path", "file", ".ext", false);
	}
#endif // CAGE_SYSTEM_WINDOWS

	{
		CAGE_TESTCASE("pathJoin");
		CAGE_TEST(pathJoin("", "") == "");
		CAGE_TEST(pathJoin(".", ".") == "");
		CAGE_TEST(pathJoin(".", "a") == "a");
		CAGE_TEST(pathJoin("a", ".") == "a");
		CAGE_TEST(pathJoin("a", "..") == "");
		CAGE_TEST(pathJoin("", "ab") == "ab");
		CAGE_TEST(pathJoin("ab", "") == "ab");
		CAGE_TEST(pathJoin("/a", "..") == "/");
		CAGE_TEST(pathJoin("abc", "") == "abc");
		CAGE_TEST(pathJoin("", "abc") == "abc");
		CAGE_TEST(pathJoin("a/", "b") == "a/b");
		CAGE_TEST(pathJoin("/ab", "") == "/ab");
		CAGE_TEST(pathJoin("/a", "b") == "/a/b");
		CAGE_TEST(pathJoin("k:/a", "..") == "k:/");
		CAGE_TEST(pathJoin("k://a", "..") == "k:/");
		CAGE_TEST(pathJoin("c:/a", "b") == "c:/a/b");
		CAGE_TEST(pathJoin("abc", "def") == "abc/def");
		CAGE_TEST(pathJoin("abc", "def/") == "abc/def");
		CAGE_TEST(pathJoin("abc/", "def") == "abc/def");
		CAGE_TEST(pathJoin("./abc", "def") == "abc/def");
		CAGE_TEST(pathJoin("a", "b/c/../../d") == "a/d");
		CAGE_TEST(pathJoin("../abc", "def") == "../abc/def");
		CAGE_TEST(pathJoin("a/b/c/../../d", "e") == "a/d/e");
		CAGE_TEST(pathJoin("abc/../def", "ghi") == "def/ghi");
		CAGE_TEST(pathJoin("a/b/c", "d/e/f") == "a/b/c/d/e/f");
		CAGE_TEST(pathJoin("../../a/b", "c") == "../../a/b/c");
		CAGE_TEST(pathJoin("a", "b/c/../../../../d") == "../d");
		CAGE_TEST(pathJoin("/abc/../def", "ghi") == "/def/ghi");
		CAGE_TEST(pathJoin("abc/./def", "ghi") == "abc/def/ghi");
		CAGE_TEST(pathJoin("a/b/c/../../../../d", "e") == "../d/e");
		CAGE_TEST_THROWN(pathJoin(".", "/"));
		CAGE_TEST_THROWN(pathJoin("a", "/a"));
		CAGE_TEST_THROWN(pathJoin("/", ".."));
		CAGE_TEST_THROWN(pathJoin("", "/ab"));
		CAGE_TEST_THROWN(pathJoin("a", "c:/"));
		CAGE_TEST_THROWN(pathJoin("k:/", ".."));
		CAGE_TEST_THROWN(pathJoin("k://", ".."));
		CAGE_TEST_THROWN(pathJoin("/a", "../.."));
		CAGE_TEST_THROWN(pathJoin("", "proto:/ab"));
		CAGE_TEST_THROWN(pathJoin("haha", "proto:/ab"));
	}

#ifndef CAGE_SYSTEM_WINDOWS
	{
		CAGE_TESTCASE("pathJoin with linux-only paths");
		CAGE_TEST(pathJoin(":", "") == ":");
		CAGE_TEST(pathJoin("", ":") == ":");
		CAGE_TEST(pathJoin("k:", "..") == "");
		CAGE_TEST(pathJoin("k:a", "..") == "");
		CAGE_TEST(pathJoin(":", "h") == "./:/h");
		CAGE_TEST(pathJoin("k:a", "b") == "k:a/b");
		CAGE_TEST(pathJoin("k", "a:b") == "k/a:b");
		CAGE_TEST(pathJoin("aa:", "bb") == "./aa:/bb");
		CAGE_TEST(pathJoin("/aa:", "bb") == "/aa:/bb");
		CAGE_TEST(pathJoin("p:/aa:", "bb") == "p:/aa:/bb");
	}
#endif // CAGE_SYSTEM_WINDOWS

	{
		CAGE_TESTCASE("path to relative");
		CAGE_TEST(pathToRel("") == "");
		CAGE_TEST(pathToRel(".") == "");
		CAGE_TEST(pathToRel("..") == "..");
		CAGE_TEST(pathToRel("abc") == "abc");
		CAGE_TEST(pathToRel("abc", "def") == "abc");
		CAGE_TEST(pathToRel(pathWorkingDir()) == "");
		CAGE_TEST(pathToRel("abc/.", "def") == "abc");
		CAGE_TEST(pathToRel("./abc", "def") == "abc");
		CAGE_TEST(pathToRel("lol:/omg") == "lol:/omg");
		CAGE_TEST(pathToRel("lol://omg") == "lol:/omg");
		CAGE_TEST(pathToRel("abc/./def", "juj") == "abc/def");
		CAGE_TEST(pathToRel(pathJoin(pathWorkingDir(), "abc")) == "abc");
		CAGE_TEST(pathToRel(pathJoin(pathWorkingDir(), "abc"), pathJoin(pathWorkingDir(), "abc")) == "");
	}

	{
		CAGE_TESTCASE("path to absolute");
		CAGE_TEST(pathToAbs("") == pathWorkingDir());
		CAGE_TEST(pathToAbs(".") == pathWorkingDir());
		CAGE_TEST(pathToAbs("lol:/abc") == "lol:/abc");
		CAGE_TEST(pathToAbs("abc") == pathJoin(pathWorkingDir(), "abc"));
		{
			const String root = [&]() -> String
			{
#ifdef CAGE_SYSTEM_WINDOWS
				return pathExtractDrive(pathWorkingDir()) + ":/";
#else
				return "/";
#endif // CAGE_SYSTEM_WINDOWS
			}();
			CAGE_TEST(pathIsValid(root) && pathIsAbs(root) && pathSimplify(root) == root);
			CAGE_TEST(pathToAbs("/") == root);
			CAGE_TEST(pathToAbs("/abc/def") == pathJoin(root, "abc/def"));
		}
	}
}
