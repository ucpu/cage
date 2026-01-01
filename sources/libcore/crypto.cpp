#include <psa/crypto.h>

#include <cage-core/crypto.h>
#include <cage-core/scopeGuard.h>

namespace cage
{
	namespace signing
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

		Signature sign(PointerRange<const char> buffer, const std::array<uint8, 32> &privateKey)
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
			checkPsa(psa_sign_message(key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)buffer.data(), buffer.size(), sig.signature.data(), sig.signature.size(), &sig_len), "crypto signing failed");
			return sig;
		}

		bool verify(PointerRange<const char> buffer, const std::array<uint8, 65> &publicKey, const Signature &signature)
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
			psa_status_t status = psa_verify_message(key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)buffer.data(), buffer.size(), signature.signature.data(), signature.signature.size());
			if (status != PSA_SUCCESS && status != PSA_ERROR_INVALID_SIGNATURE)
				checkPsa(status, "crypto verification process failed");
			return status == PSA_SUCCESS;
		}
	}
}
