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
	}
}