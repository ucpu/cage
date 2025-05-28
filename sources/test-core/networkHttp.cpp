#include "main.h"

#include <algorithm>

#include <cage-core/networkHttp.h>

void testNetworkHttp()
{
	CAGE_TESTCASE("network http");

	{
		CAGE_TESTCASE("get github");
		const auto r = http(HttpRequest{ .url = "https://github.com/ucpu/cage" });
		CAGE_TEST((r.body.find("game engine", 0) != r.body.npos)); // verify that the returned body contains specific text
		CAGE_TEST(r.statusCode == 200);
	}

	{
		CAGE_TESTCASE("invalid protocol");
		CAGE_TEST_THROWN((http(HttpRequest{ .url = "sptth://github.com" })));
	}

	{
		CAGE_TESTCASE("invalid url");
		CAGE_TEST_THROWN((http(HttpRequest{ .url = "https://nonexistenturlforsureyouknowiamsure.com" })));
	}
}
