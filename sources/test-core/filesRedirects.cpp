#include <algorithm>
#include <vector>

#include "main.h"

#include <cage-core/files.h>

namespace
{
	void compareSets(Holder<PointerRange<String>> &&setA, std::vector<String> setB)
	{
		CAGE_TEST(setA.size() == setB.size());
		std::sort(setA.begin(), setA.end());
		std::sort(setB.begin(), setB.end());
		for (uint32 i = 0; i < setA.size(); i++)
		{
			CAGE_TEST(setA[i] == pathToAbs(setA[i])); // all paths returned from directory listing must be absolute
			CAGE_TEST(setA[i] == pathToAbs(setB[i])); // the two sets must be same
		}
	}

	void testRedirectsEntry()
	{
		{
			CAGE_TESTCASE("basic redirects");
			pathCreateRedirect("testdir/redirect", "testdir/target");
			writeFile("testdir/redirect/testfile.txt")->writeLine("hello there");
			CAGE_TEST(any(pathType("testdir/redirect/testfile.txt") & PathTypeFlags::File));
			CAGE_TEST(any(pathType("testdir/target/testfile.txt") & PathTypeFlags::File));
			compareSets(pathListDirectory("testdir"), { "testdir/target", "testdir/redirect" }); // make sure the redirection is also found
			compareSets(pathListDirectory("testdir/target"), { "testdir/target/testfile.txt" });
			compareSets(pathListDirectory("testdir/redirect"), { "testdir/redirect/testfile.txt" }); // preserve original path, not the redirected one
			pathRemoveRedirect("testdir/redirect");
			CAGE_TEST(pathListRedirects().empty());
		}

		{
			CAGE_TESTCASE("invalid redirects");
			CAGE_TEST_THROWN(pathCreateRedirect("testdir/aaa", "testdir/aaa/bbb/ccc"));
			CAGE_TEST(pathListRedirects().empty());
			CAGE_TEST_THROWN(pathCreateRedirect("testdir/aaa/bbb/ccc", "testdir/aaa"));
			CAGE_TEST(pathListRedirects().empty());
			pathCreateRedirect("testdir/aaa", "testdir/aaa");
			CAGE_TEST(pathListRedirects().empty());
			pathCreateRedirect("testdir/aaa", "testdir/bbb");
			CAGE_TEST(pathListRedirects().size() == 1);
			CAGE_TEST_THROWN(pathCreateRedirect("testdir/aaa", "testdir/bbb")); // same redirect repeated
			CAGE_TEST(pathListRedirects().size() == 1);
			pathCreateRedirect("testdir/bbb", "testdir/ccc");
			CAGE_TEST(pathListRedirects().size() == 2);
			CAGE_TEST(pathResolveRedirects("testdir/aaa") == pathToAbs("testdir/ccc")); // redirects may chain
			pathRemoveRedirect("testdir/aaa");
			CAGE_TEST(pathListRedirects().size() == 1);
			CAGE_TEST(pathResolveRedirects("testdir/aaa") == "testdir/aaa"); // no redirect (preserves original path, not absolute)
			pathCreateRedirect("testdir/aaa", "testdir/bbb");
			CAGE_TEST(pathListRedirects().size() == 2);
			CAGE_TEST(pathResolveRedirects("testdir/aaa") == pathToAbs("testdir/ccc")); // redirects may chain, irrespective of order of creation
			pathRemoveRedirect("testdir/aaa");
			pathRemoveRedirect("testdir/bbb");
			pathRemoveRedirect("testdir/ddd"); // removing non-existent redirect is acceptable
			CAGE_TEST(pathListRedirects().empty());
		}
	}
}

void testFilesRedirects()
{
	CAGE_TESTCASE("files redirects");

	pathRemove("testdir");

	// sanity check
	CAGE_TEST(pathListRedirects().empty());

	testRedirectsEntry();

	// clean up
	for (const auto &it : pathListRedirects())
		pathRemoveRedirect(it.first);
	CAGE_TEST(pathListRedirects().empty());
}
