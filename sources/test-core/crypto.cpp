#include "main.h"

#include <cage-core/crypto.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>

void testCrypto()
{
	CAGE_TESTCASE("crypto");

	MemoryBuffer buffer;
	buffer.resize(randomRange(300, 500));
	for (uint32 i = 0; i < buffer.size(); i++)
		buffer.data()[i] = randomRange(0, 255);

	const auto pair = signing::generateKeyPair();
	const auto signature = signing::sign(buffer, pair.privateKey);
	CAGE_TEST(signing::verify(buffer, pair.publicKey, signature));

	buffer.data()[13]++;
	CAGE_TEST(!signing::verify(buffer, pair.publicKey, signature));
}
