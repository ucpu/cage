#ifndef guard_crypto_h_dt5uqs5dfc7zh5kl
#define guard_crypto_h_dt5uqs5dfc7zh5kl

#include <array>

#include <cage-core/core.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	namespace signing
	{
		using PrivateKey = std::array<uint8, 32>;
		using PublicKey = std::array<uint8, 65>;
		using Signature = std::array<uint8, 64>;

		struct KeyPair
		{
			PrivateKey privateKey = {};
			PublicKey publicKey = {};
		};

		CAGE_CORE_API KeyPair generateKeyPair();

		CAGE_CORE_API Signature sign(PointerRange<const char> buffer, const PrivateKey &privateKey);

		CAGE_CORE_API bool verify(PointerRange<const char> buffer, const PublicKey &publicKey, const Signature &signature);
	}

	namespace encryption
	{
		using PrivateKey = std::array<uint8, 2400>;
		using PublicKey = std::array<uint8, 800>;

		struct KeyPair
		{
			PrivateKey privateKey = {};
			PublicKey publicKey = {};
		};

		CAGE_CORE_API KeyPair generateKeyPair();

		CAGE_CORE_API MemoryBuffer encrypt(PointerRange<const char> buffer, const PublicKey &publicKey);

		CAGE_CORE_API MemoryBuffer decrypt(PointerRange<const char> buffer, const PrivateKey &privateKey);
	}
}

#endif // guard_crypto_h_dt5uqs5dfc7zh5kl
