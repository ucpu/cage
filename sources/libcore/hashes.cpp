#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha3.h>
#include <mbedtls/sha512.h>

#include <cage-core/hashes.h>

namespace cage
{
	std::array<uint8, 20> hashSha1(PointerRange<const char> data)
	{
		std::array<uint8, 20> res = {};
		auto status = mbedtls_sha1((const uint8 *)data.data(), data.size(), res.data());
		if (status != 0)
			CAGE_THROW_ERROR(Exception, "sha1 failure");
		return res;
	}

	std::array<uint8, 32> hashSha2_256(PointerRange<const char> data)
	{
		std::array<uint8, 32> res = {};
		auto status = mbedtls_sha256((const uint8 *)data.data(), data.size(), res.data(), 0);
		if (status != 0)
			CAGE_THROW_ERROR(Exception, "sha2_256 failure");
		return res;
	}

	std::array<uint8, 64> hashSha2_512(PointerRange<const char> data)
	{
		std::array<uint8, 64> res = {};
		auto status = mbedtls_sha512((const uint8 *)data.data(), data.size(), res.data(), 0);
		if (status != 0)
			CAGE_THROW_ERROR(Exception, "sha2_512 failure");
		return res;
	}

	std::array<uint8, 32> hashSha3_256(PointerRange<const char> data)
	{
		std::array<uint8, 32> res = {};
		auto status = mbedtls_sha3(MBEDTLS_SHA3_256, (const uint8 *)data.data(), data.size(), res.data(), res.size());
		if (status != 0)
			CAGE_THROW_ERROR(Exception, "sha3_256 failure");
		return res;
	}

	std::array<uint8, 64> hashSha3_512(PointerRange<const char> data)
	{
		std::array<uint8, 64> res = {};
		auto status = mbedtls_sha3(MBEDTLS_SHA3_512, (const uint8 *)data.data(), data.size(), res.data(), res.size());
		if (status != 0)
			CAGE_THROW_ERROR(Exception, "sha3_512 failure");
		return res;
	}

	String hashToHexadecimal(PointerRange<const uint8> data)
	{
		static constexpr const char hexChars[] = "0123456789abcdef";
		String hex = "";
		for (uint8 d : data)
		{
			hex += String(hexChars[(d >> 4) & 0xF]);
			hex += String(hexChars[d & 0xF]);
		}
		return hex;
	}

	void hexadecimalToHash(const String &inputHexadecimal, PointerRange<uint8> outputHash)
	{
		if (outputHash.size() * 2 != inputHexadecimal.size())
			CAGE_THROW_ERROR(Exception, "size mismatch for hexadecimal hash");

		const auto &hexValue = [](char c) -> uint8
		{
			if (c >= '0' && c <= '9')
				return uint8(c - '0');
			if (c >= 'a' && c <= 'f')
				return uint8(c - 'a' + 10);
			if (c >= 'A' && c <= 'F')
				return uint8(c - 'A' + 10);
			CAGE_THROW_ERROR(Exception, "invalid hexadecimal character");
		};

		for (uint32 i = 0; i < outputHash.size(); ++i)
		{
			uint8 high = hexValue(inputHexadecimal[i * 2]);
			uint8 low = hexValue(inputHexadecimal[i * 2 + 1]);
			outputHash[i] = uint8((high << 4) | low);
		}
	}

	String hashToBase64(PointerRange<const uint8> data)
	{
		// https://gist.github.com/tomykaira/f0fd86b6c73063283afe550bc5d77594
		// modified

		static constexpr const char sEncodingTable[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

		String ret;
		ret.rawLength() = 4 * ((data.size() + 2) / 3);
		ret.rawData()[ret.length()] = 0;
		char *p = ret.rawData();

		uint32 i = 0;
		for (; i < data.size() - 2; i += 3)
		{
			*p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
			*p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
			*p++ = sEncodingTable[((data[i + 1] & 0xF) << 2) | ((int)(data[i + 2] & 0xC0) >> 6)];
			*p++ = sEncodingTable[data[i + 2] & 0x3F];
		}
		if (i < data.size())
		{
			*p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
			if (i == (data.size() - 1))
			{
				*p++ = sEncodingTable[((data[i] & 0x3) << 4)];
				*p++ = '=';
			}
			else
			{
				*p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
				*p++ = sEncodingTable[((data[i + 1] & 0xF) << 2)];
			}
			*p++ = '=';
		}

		return ret;
	}
}
