#ifndef guard_crypto_h_dt5uqs5dfc7zh5kl
#define guard_crypto_h_dt5uqs5dfc7zh5kl

#include <array>

#include <cage-core/core.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	namespace signing
	{
		struct KeyPair
		{
			std::array<uint8, 32> privateKey;
			std::array<uint8, 65> publicKey;
		};

		CAGE_CORE_API KeyPair generateKeyPair();

		struct Signature
		{
			std::array<uint8, 64> signature = {};
		};

		CAGE_CORE_API Signature sign(PointerRange<const char> buffer, const std::array<uint8, 32> &privateKey);

		CAGE_CORE_API bool verify(PointerRange<const char> buffer, const std::array<uint8, 65> &publicKey, const Signature &signature);
	}

	/*
	namespace encryption
	{
		struct KeyPair
		{
			std::array<uint8, 32> privateKey;
			std::array<uint8, 65> publicKey;
		};

		CAGE_CORE_API KeyPair generateKeyPair();

		CAGE_CORE_API MemoryBuffer encrypt(PointerRange<const char> buffer, const std::array<uint8, 65> &publicKey);

		CAGE_CORE_API MemoryBuffer decrypt(PointerRange<const char> buffer, const std::array<uint8, 32> &privateKey);
	}
	*/
}

#endif // guard_crypto_h_dt5uqs5dfc7zh5kl
