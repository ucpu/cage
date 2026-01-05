#include <array>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>

#include <mbedtls/psa_util.h>
#include <psa/crypto.h>

#include <cage-core/crypto.h>
#include <cage-core/scopeGuard.h>

namespace cage
{
	namespace
	{
		void checkPsa(psa_status_t status, StringPointer msg)
		{
			if (status != PSA_SUCCESS)
			{
				CAGE_LOG_THROW(Stringizer() + "status: " + status);
				CAGE_THROW_ERROR(Exception, msg);
			}
		}

		void initPsa()
		{
			static psa_status_t status = psa_crypto_init();
			checkPsa(status, "failed to initialize crypto");
		}
	}

	namespace signing
	{
		KeyPair generateKeyPair()
		{
			initPsa();
			psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
			psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_EXPORT);
			psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
			psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
			psa_set_key_bits(&attributes, 256);
			psa_key_id_t key_id = 0;
			ScopeGuard guard([&]() { psa_destroy_key(key_id); });
			checkPsa(psa_generate_key(&attributes, &key_id), "crypto key generation failed");
			KeyPair kp;
			std::size_t exported_len = 0;
			checkPsa(psa_export_key(key_id, kp.privateKey.data(), kp.privateKey.size(), &exported_len), "crypto private key export failed");
			checkPsa(psa_export_public_key(key_id, kp.publicKey.data(), kp.publicKey.size(), &exported_len), "crypto public key export failed");
			return kp;
		}

		Signature sign(PointerRange<const char> buffer, const PrivateKey &privateKey)
		{
			initPsa();
			psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
			psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
			psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
			psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
			psa_set_key_bits(&attributes, 256);
			psa_key_id_t key_id = 0;
			ScopeGuard guard([&]() { psa_destroy_key(key_id); });
			checkPsa(psa_import_key(&attributes, privateKey.data(), privateKey.size(), &key_id), "crypto private key import failed");
			std::size_t sig_len = 0;
			Signature sig;
			checkPsa(psa_sign_message(key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)buffer.data(), buffer.size(), sig.data(), sig.size(), &sig_len), "crypto signing failed");
			return sig;
		}

		bool verify(PointerRange<const char> buffer, const PublicKey &publicKey, const Signature &signature)
		{
			initPsa();
			psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
			psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
			psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
			psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
			psa_set_key_bits(&attributes, 256);
			psa_key_id_t key_id = 0;
			ScopeGuard guard([&]() { psa_destroy_key(key_id); });
			checkPsa(psa_import_key(&attributes, publicKey.data(), publicKey.size(), &key_id), "crypto public key import failed");
			// return false on PSA_ERROR_INVALID_SIGNATURE
			psa_status_t status = psa_verify_message(key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)buffer.data(), buffer.size(), signature.data(), signature.size());
			if (status != PSA_SUCCESS && status != PSA_ERROR_INVALID_SIGNATURE)
				checkPsa(status, "crypto verification process failed");
			return status == PSA_SUCCESS;
		}
	}

	namespace encryption
	{
		namespace
		{
			struct DynamicLengthTag
			{};

			struct KeyHandle : private Immovable
			{
				psa_key_handle_t h = 0;

				// generate new key
				KeyHandle(psa_key_type_t type, psa_key_usage_t usage, psa_algorithm_t alg, uint32 bits)
				{
					psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
					psa_set_key_type(&attr, type);
					psa_set_key_bits(&attr, bits);
					psa_set_key_usage_flags(&attr, usage);
					psa_set_key_algorithm(&attr, alg);
					checkPsa(psa_generate_key(&attr, &h), "failed to generate crypto key");
				}

				// import rsa key
				KeyHandle(DynamicLengthTag, PointerRange<const uint8> data, psa_key_type_t type, psa_key_usage_t usage, psa_algorithm_t alg = PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256))
				{
					CAGE_ASSERT(data.size() > 4);
					uint32 size = *(uint32 *)data.data();
					if (size + 4 > data.size())
						CAGE_THROW_ERROR(Exception, "crypto key with invalid length");
					import(data.data() + 4, size, type, usage, alg, 3072);
				}

				// import aes key
				KeyHandle(PointerRange<const uint8> data, psa_key_type_t type, psa_key_usage_t usage, psa_algorithm_t alg, uint32 bits) { import(data.data(), data.size(), type, usage, alg, bits); }

				~KeyHandle() { psa_destroy_key(h); }

			private:
				void import(const uint8 *data, uint32 size, psa_key_type_t type, psa_key_usage_t usage, psa_algorithm_t alg, uint32 bits)
				{
					psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
					psa_set_key_type(&attr, type);
					psa_set_key_bits(&attr, bits);
					psa_set_key_usage_flags(&attr, usage);
					psa_set_key_algorithm(&attr, alg);
					checkPsa(psa_import_key(&attr, data, size, &h), "failed to import crypto key");
				}
			};
		}

		KeyPair generateKeyPair()
		{
			initPsa();
			KeyHandle key(PSA_KEY_TYPE_RSA_KEY_PAIR, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_ENCRYPT, PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256), 3072);
			KeyPair pair;
			size_t olen;
			checkPsa(psa_export_public_key(key.h, (uint8 *)pair.publicKey.data() + 4, pair.publicKey.size() - 4, &olen), "failed to export crypto key");
			CAGE_ASSERT(olen < pair.publicKey.size() - 4);
			*(uint32 *)pair.publicKey.data() = numeric_cast<uint32>(olen);
			checkPsa(psa_export_key(key.h, (uint8 *)pair.privateKey.data() + 4, pair.privateKey.size() - 4, &olen), "failed to export crypto key");
			CAGE_ASSERT(olen < pair.privateKey.size() - 4);
			*(uint32 *)pair.privateKey.data() = numeric_cast<uint32>(olen);
			return pair;
		}

		MemoryBuffer encrypt(PointerRange<const char> plaintext, const PublicKey &publicKey)
		{
			initPsa();
			KeyHandle rsa(DynamicLengthTag(), publicKey, PSA_KEY_TYPE_RSA_PUBLIC_KEY, PSA_KEY_USAGE_ENCRYPT);
			std::array<uint8, 32> aes_key;
			checkPsa(psa_generate_random(aes_key.data(), aes_key.size()), "failed to generate random crypto key");
			std::array<uint8, 512> enc_key;
			std::size_t enc_key_len;
			checkPsa(psa_asymmetric_encrypt(rsa.h, PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256), aes_key.data(), aes_key.size(), nullptr, 0, enc_key.data(), enc_key.size(), &enc_key_len), "failed to encrypt");
			CAGE_ASSERT(enc_key_len < enc_key.size());
			KeyHandle aes(aes_key, PSA_KEY_TYPE_AES, PSA_KEY_USAGE_ENCRYPT, PSA_ALG_GCM, 256);
			std::array<uint8, 12> nonce;
			checkPsa(psa_generate_random(nonce.data(), nonce.size()), "failed to generate crypto nonce");
			MemoryBuffer out;
			out.resize(4 + enc_key_len + nonce.size() + plaintext.size() + 16);
			uint8 *p = (uint8 *)out.data();
			uint32 ek = enc_key_len;
			std::memcpy(p, &ek, 4);
			p += 4;
			std::memcpy(p, enc_key.data(), enc_key_len);
			p += enc_key_len;
			std::memcpy(p, nonce.data(), nonce.size());
			p += nonce.size();
			std::size_t olen;
			checkPsa(psa_aead_encrypt(aes.h, PSA_ALG_GCM, nonce.data(), nonce.size(), nullptr, 0, (const uint8 *)plaintext.data(), plaintext.size(), (uint8 *)p, plaintext.size() + 16, &olen), "failed to encrypt");
			out.resize(4 + enc_key_len + nonce.size() + olen);
			return out;
		}

		MemoryBuffer decrypt(PointerRange<const char> ciphertext, const PrivateKey &privateKey)
		{
			initPsa();
			const uint8 *p = (const uint8 *)ciphertext.data();
			uint32_t enc_key_len;
			if (ciphertext.size() < 4)
				CAGE_THROW_ERROR(Exception, "failed to decrypt: buffer too small");
			std::memcpy(&enc_key_len, p, 4);
			if (enc_key_len > ciphertext.size() - 4 || ciphertext.size() < 4 + enc_key_len + 12 + 16)
				CAGE_THROW_ERROR(Exception, "failed to decrypt: buffer too small");
			p += 4;
			KeyHandle rsa(DynamicLengthTag(), privateKey, PSA_KEY_TYPE_RSA_KEY_PAIR, PSA_KEY_USAGE_DECRYPT);
			std::array<uint8, 32> aes_key;
			std::size_t aes_len;
			checkPsa(psa_asymmetric_decrypt(rsa.h, PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256), p, enc_key_len, nullptr, 0, aes_key.data(), aes_key.size(), &aes_len), "failed to decrypt");
			CAGE_ASSERT(aes_len == aes_key.size());
			p += enc_key_len;
			KeyHandle aes(aes_key, PSA_KEY_TYPE_AES, PSA_KEY_USAGE_DECRYPT, PSA_ALG_GCM, 256);
			const uint8 *nonce = p;
			p += 12;
			std::size_t ct_len = ciphertext.size() - (p - (const uint8 *)ciphertext.data());
			MemoryBuffer out;
			out.resize(ct_len);
			std::size_t olen;
			checkPsa(psa_aead_decrypt(aes.h, PSA_ALG_GCM, nonce, 12, nullptr, 0, p, ct_len, (uint8 *)out.data(), out.size(), &olen), "failed to decrypt");
			out.resize(olen);
			return out;
		}
	}
}
