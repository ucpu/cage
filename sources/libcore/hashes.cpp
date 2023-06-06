#include <cage-core/hashes.h>

#include <algorithm>
#include <array>
#include <vector>

namespace cage
{
	std::array<uint8, 20> hashSha1(PointerRange<const char> data)
	{
		// https://thealgorithms.github.io/C-Plus-Plus/d8/d7a/sha1_8cpp.html
		// modified

		const auto &leftRotate32bits = [](uint32 n, uint32 rotate) -> uint32 { return (n << rotate) | (n >> (32 - rotate)); };

		uint32 h0 = 0x67452301, a = 0;
		uint32 h1 = 0xEFCDAB89, b = 0;
		uint32 h2 = 0x98BADCFE, c = 0;
		uint32 h3 = 0x10325476, d = 0;
		uint32 h4 = 0xC3D2E1F0, e = 0;

		const uint32 input_size = numeric_cast<uint32>(data.size());
		const uint64 padded_message_size = input_size % 64 < 56 ? input_size + 64 - (input_size % 64) : input_size + 128 - (input_size % 64);
		std::vector<uint8> padded_message;
		padded_message.resize(padded_message_size);
		std::copy((const uint8 *)data.begin(), (const uint8 *)data.end(), padded_message.begin());
		padded_message[input_size] = 1 << 7; // 10000000
		for (uint64 i = input_size; i % 64 != 56; i++)
		{
			if (i == input_size)
				continue;
			padded_message[i] = 0;
		}
		for (uint8 i = 0; i < 8; i++)
			padded_message[padded_message_size - 8 + i] = (uint64(input_size * 8) >> (56 - 8 * i)) & 0xFF;

		std::array<uint32, 80> blocks = {};
		for (uint64 chunk = 0; chunk * 64 < padded_message_size; chunk++)
		{
			for (uint8 bid = 0; bid < 16; bid++)
			{
				blocks[bid] = 0;
				for (uint8 cid = 0; cid < 4; cid++)
					blocks[bid] = (blocks[bid] << 8) + padded_message[chunk * 64 + bid * 4 + cid];
				for (uint8 i = 16; i < 80; i++)
					blocks[i] = leftRotate32bits(blocks[i - 3] ^ blocks[i - 8] ^ blocks[i - 14] ^ blocks[i - 16], 1);
			}

			a = h0;
			b = h1;
			c = h2;
			d = h3;
			e = h4;

			for (uint8 i = 0; i < 80; i++)
			{
				uint32 F = 0, g = 0;
				if (i < 20)
				{
					F = (b & c) | ((~b) & d);
					g = 0x5A827999;
				}
				else if (i < 40)
				{
					F = b ^ c ^ d;
					g = 0x6ED9EBA1;
				}
				else if (i < 60)
				{
					F = (b & c) | (b & d) | (c & d);
					g = 0x8F1BBCDC;
				}
				else
				{
					F = b ^ c ^ d;
					g = 0xCA62C1D6;
				}

				const uint32 temp = leftRotate32bits(a, 5) + F + e + g + blocks[i];
				e = d;
				d = c;
				c = leftRotate32bits(b, 30);
				b = a;
				a = temp;
			}

			h0 += a;
			h1 += b;
			h2 += c;
			h3 += d;
			h4 += e;
		}

		std::array<uint8, 20> sig = {};
		for (uint8 i = 0; i < 4; i++)
		{
			sig[i] = (h0 >> (24 - 8 * i)) & 0xFF;
			sig[i + 4] = (h1 >> (24 - 8 * i)) & 0xFF;
			sig[i + 8] = (h2 >> (24 - 8 * i)) & 0xFF;
			sig[i + 12] = (h3 >> (24 - 8 * i)) & 0xFF;
			sig[i + 16] = (h4 >> (24 - 8 * i)) & 0xFF;
		}
		return sig;
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
