#include "main.h"

#include <cage-core/hashes.h>

void testHashes()
{
	CAGE_TESTCASE("hashes");

	{
		CAGE_TESTCASE("sha1");

		{
			const auto r = hashSha1("");
			CAGE_TEST(hashToHexadecimal(r) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
			CAGE_TEST(hashToBase64(r) == "2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
		}

		{
			const auto r = hashSha1("The quick brown fox jumps over the lazy dog");
			CAGE_TEST(hashToHexadecimal(r) == "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
			CAGE_TEST(hashToBase64(r) == "L9ThxnotKPzthJ7hu3bnORuT6xI=");
		}

		{
			const auto r = hashSha1("The quick brown fox jumps over the lazy cog");
			CAGE_TEST(hashToHexadecimal(r) == "de9f2c7fd25e1b3afad3e85a0bd17d9b100db4b3");
			CAGE_TEST(hashToBase64(r) == "3p8sf9JeGzr60+haC9F9mxANtLM=");
		}
	}
}
