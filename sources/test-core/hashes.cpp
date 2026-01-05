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

			std::array<uint8, 20> v = {};
			hexadecimalToHash(hashToHexadecimal(r), v);
			CAGE_TEST(r == v);
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

	{
		CAGE_TESTCASE("sha2");

		{
			const auto r = hashSha2_512("");
			CAGE_TEST(hashToHexadecimal(r) == "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");

			std::array<uint8, 64> v = {};
			hexadecimalToHash(hashToHexadecimal(r), v);
			CAGE_TEST(r == v);
		}

		{
			const auto r = hashSha2_512("The quick brown fox jumps over the lazy dog");
			CAGE_TEST(hashToHexadecimal(r) == "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6");
		}
	}

	{
		CAGE_TESTCASE("sha3");

		{
			const auto r = hashSha3_512("");
			CAGE_TEST(hashToHexadecimal(r) == "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26");
		}

		{
			const auto r = hashSha3_512("The quick brown fox jumps over the lazy dog");
			CAGE_TEST(hashToHexadecimal(r) == "01dedd5de4ef14642445ba5f5b97c15e47b9ad931326e4b0727cd94cefc44fff23f07bf543139939b49128caf436dc1bdee54fcb24023a08d9403f9b4bf0d450");
		}
	}
}
