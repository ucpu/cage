#include <vector>
#include <algorithm>

#define CAGE_EXPORT
#include <cage-core/core.h>

namespace cage
{
	namespace privat
	{
		uint32 encodeUrlBase(char *pStart, const char *pSrc, uint32 length)
		{
			static const bool SAFE[256] =
			{
				/*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
				/* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* 2 */ 0,0,0,0, 0,0,0,0, 1,1,0,1, 0,1,1,0, // (, ), +, -, .
				/* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

				/* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
				/* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,1, // _
				/* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
				/* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,1,0, // ~

				/* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

				/* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
				/* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
			};
			static const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
			char * pEnd = pStart;
			const char * const SRC_END = pSrc + length;
			for (; pSrc < SRC_END; ++pSrc)
			{
				if (SAFE[(unsigned char)*pSrc])
					*pEnd++ = *pSrc;
				else
				{
					*pEnd++ = '%';
					*pEnd++ = DEC2HEX[((unsigned char)*pSrc) >> 4];
					*pEnd++ = DEC2HEX[((unsigned char)*pSrc) & 0x0F];
				}
			}
			return numeric_cast<uint32>(pEnd - pStart);
		}

		uint32 decodeUrlBase(char *pStart, const char *pSrc, uint32 length)
		{
			static const char HEX2DEC[256] =
			{
				/*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
				/* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

				/* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

				/* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

				/* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
				/* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
			};
			const char * const SRC_END = pSrc + length;
			const char * const SRC_LAST_DEC = SRC_END - 2;
			char * pEnd = pStart;
			while (pSrc < SRC_LAST_DEC)
			{
				if (*pSrc == '%')
				{
					char dec1, dec2;
					if (-1 != (dec1 = HEX2DEC[(unsigned char)*(pSrc + 1)])
						&& -1 != (dec2 = HEX2DEC[(unsigned char)*(pSrc + 2)]))
					{
						*pEnd++ = (dec1 << 4) + dec2;
						pSrc += 3;
						continue;
					}
				}
				*pEnd++ = *pSrc++;
			}
			while (pSrc < SRC_END)
				*pEnd++ = *pSrc++;
			return numeric_cast<uint32>(pEnd - pStart);
		}

		void encodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength)
		{
			char tmp[4096];
			uint32 len = encodeUrlBase(tmp, dataIn, currentIn);
			if (len > maxLength)
				CAGE_THROW_ERROR(exception, "string truncation");
			detail::memcpy(dataOut, tmp, len);
			currentOut = len;
		}

		void decodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength)
		{
			char tmp[4096];
			uint32 len = decodeUrlBase(tmp, dataIn, currentIn);
			if (len > maxLength)
				CAGE_THROW_ERROR(exception, "string truncation");
			detail::memcpy(dataOut, tmp, len);
			currentOut = len;
		}

		namespace
		{
			bool isOrdered(const char *data, uint32 current)
			{
				std::vector<char> data2(data, data + current);
				uint32 current2 = current;
				stringSortAndUnique(data2.data(), current2);
				if (current2 != current)
					return false;
				return detail::memcmp(data, data2.data(), current) == 0;
			}
		}

		void stringReplace(char *data, uint32 &current, uint32 maxLength, const char *what, uint32 whatLen, const char *with, uint32 withLen)
		{
			if (whatLen == 0)
				return;
			uint32 pos = 0;
			while (true)
			{
				pos = stringFind(data, current, what, whatLen, pos);
				if (pos == m)
					break;
				if (current + withLen - whatLen > maxLength)
					CAGE_THROW_ERROR(exception, "string truncation");
				detail::memmove(data + pos + withLen, data + pos + whatLen, current - pos - whatLen);
				detail::memcpy(data + pos, with, withLen);
				current += withLen - whatLen;
				pos += withLen - whatLen + 1;
			}
		}

		void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right)
		{
			CAGE_ASSERT_RUNTIME(isOrdered(what, whatLen));
			if (whatLen == 0)
				return;
			if (!left && !right)
				return;
			if (right)
			{
				uint32 p = 0;
				while (p < current && stringContains(what, whatLen, data[current - p - 1]))
					p++;
				current -= p;
			}
			if (left)
			{
				uint32 p = 0;
				while (p < current && stringContains(what, whatLen, data[p]))
					p++;
				current -= p;
				if (p > 0)
					detail::memmove(data, data + p, current);
			}
		}

		void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen)
		{
			CAGE_ASSERT_RUNTIME(retLen == 0);
			CAGE_ASSERT_RUNTIME(isOrdered(what, whatLen));
			if (whatLen == 0)
				return;
			for (uint32 i = 0; i < current; i++)
			{
				if (stringContains(what, whatLen, data[i]))
				{
					detail::memcpy(ret, data, i);
					detail::memmove(data, data + i + 1, current - i - 1);
					retLen = i;
					current -= i + 1;
					return;
				}
			}
			detail::memcpy(ret, data, current);
			std::swap(current, retLen);
		}

		uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset)
		{
			if (whatLen == 0 || offset + whatLen >= current)
				return m;
			uint32 end = current - whatLen + 1;
			for (uint32 i = offset; i < end; i++)
				if (detail::memcmp(data + i, what, whatLen) == 0)
					return i;
			return m;
		}

		void stringSortAndUnique(char *data, uint32 &current)
		{
			std::sort(data, data + current);
			auto it = std::unique(data, data + current);
			current = numeric_cast<uint32>(it - data);
		}

		bool stringContains(const char *data, uint32 current, char what)
		{
			CAGE_ASSERT_RUNTIME(isOrdered(data, current));
			return std::binary_search(data, data + current, what);
		}
	}
}
