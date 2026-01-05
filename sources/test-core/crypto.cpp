#include "main.h"

#include <cage-core/crypto.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>

void testCrypto()
{
	CAGE_TESTCASE("crypto");

	{
		CAGE_TESTCASE("signing");

		MemoryBuffer buffer;
		buffer.resize(randomRange(300, 500));
		for (uint32 i = 0; i < buffer.size(); i++)
			buffer.data()[i] = randomRange(0, 255);

		const auto pair = signing::generateKeyPair();
		const auto signature = signing::sign(buffer, pair.privateKey);
		CAGE_TEST(signing::verify(buffer, pair.publicKey, signature));

		buffer.data()[13]++; // modify random byte
		CAGE_TEST(!signing::verify(buffer, pair.publicKey, signature));
	}

	{
		CAGE_TESTCASE("encryption");

		MemoryBuffer buffer;
		buffer.resize(randomRange(300, 500));
		for (uint32 i = 0; i < buffer.size(); i++)
			buffer.data()[i] = randomRange(0, 255);

		const auto pair = encryption::generateKeyPair();

		MemoryBuffer enc = encryption::encrypt(buffer, pair.publicKey);
		MemoryBuffer dec = encryption::decrypt(enc, pair.privateKey);
		CAGE_TEST(dec.size() == buffer.size() && detail::memcmp(dec.data(), buffer.data(), buffer.size()) == 0);

		enc.data()[enc.size() / 2]++; // modify random byte
		CAGE_TEST_THROWN(encryption::decrypt(enc, pair.privateKey));
	}
}
