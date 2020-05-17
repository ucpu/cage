#include <cage-core/core.h>
#include <cage-core/macros.h>

#include <vector>
#include <algorithm>
#include <cctype> // std::isspace
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace cage
{
	namespace detail
	{
		void *memset(void *ptr, int value, uintPtr num) noexcept
		{
			return std::memset(ptr, value, num);
		}

		void *memcpy(void *destination, const void *source, uintPtr num) noexcept
		{
			return std::memcpy(destination, source, num);
		}

		void *memmove(void *destination, const void *source, uintPtr num) noexcept
		{
			return std::memmove(destination, source, num);
		}

		int memcmp(const void *ptr1, const void *ptr2, uintPtr num) noexcept
		{
			return std::memcmp(ptr1, ptr2, num);
		}
	}

	namespace privat
	{
		namespace
		{
			template<class T>
			void genericScan(const char *s, T &value)
			{
				errno = 0;
				char *e = nullptr;
				if (std::numeric_limits<T>::is_signed)
				{
					sint64 v = std::strtoll(s, &e, 10);
					value = (T)v;
					if (v < std::numeric_limits<T>::min() || v > std::numeric_limits<T>::max())
						e = nullptr;
				}
				else
				{
					uint64 v = std::strtoull(s, &e, 10);
					value = (T)v;
					if (*s == '-' || v < std::numeric_limits<T>::min() || v > std::numeric_limits<T>::max())
						e = nullptr;
				}
				if (!*s || !e || *e != 0 || std::isspace(*s) || errno != 0)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(Exception, "fromString failed");
				}
			}

			template<>
			void genericScan<sint64>(const char *s, sint64 &value)
			{
				errno = 0;
				char *e = nullptr;
				value = std::strtoll(s, &e, 10);
				if (!*s || !e || *e != 0 || std::isspace(*s) || errno != 0)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(Exception, "fromString failed");
				}
			}

			template<>
			void genericScan<uint64>(const char *s, uint64 &value)
			{
				errno = 0;
				char *e = nullptr;
				value = std::strtoull(s, &e, 10);
				if (!*s || !e || *s == '-' || *e != 0 || std::isspace(*s) || errno != 0)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(Exception, "fromString failed");
				}
			}

			template<>
			void genericScan<double>(const char *s, double &value)
			{
				errno = 0;
				char *e = nullptr;
				double v = std::strtod(s, &e);
				if (!*s || !e || *e != 0 || std::isspace(*s) || errno != 0)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(Exception, "fromString failed");
				}
				value = v;
			}

			template<>
			void genericScan<float>(const char *s, float &value)
			{
				double v;
				genericScan(s, v);
				if (v < std::numeric_limits<float>::lowest() || v > std::numeric_limits<float>::max())
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "input string: '" + s + "'");
					CAGE_THROW_ERROR(Exception, "fromString failed");
				}
				value = (float)v;
			}

			void stringSortAndUnique(char *data, uint32 &current)
			{
				std::sort(data, data + current);
				auto it = std::unique(data, data + current);
				current = numeric_cast<uint32>(it - data);
			}

			bool isOrdered(const char *data, uint32 current)
			{
				std::vector<char> data2(data, data + current);
				uint32 current2 = current;
				stringSortAndUnique(data2.data(), current2);
				if (current2 != current)
					return false;
				return std::memcmp(data, data2.data(), current) == 0;
			}

			bool stringContains(const char *data, uint32 current, char what)
			{
				CAGE_ASSERT(isOrdered(data, current));
				return std::binary_search(data, data + current, what);
			}

			uint32 encodeUrlBase(char *pStart, const char *pSrc, uint32 length)
			{
				static constexpr bool SAFE[256] =
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
				static constexpr char DEC2HEX[16 + 1] = "0123456789ABCDEF";
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
				static constexpr char HEX2DEC[256] =
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
		}

#define GCHL_GENERATE(TYPE, SPEC) \
		uint32 toString(char *s, TYPE value) \
		{ sint32 ret = std::sprintf(s, CAGE_STRINGIZE(SPEC), value); if (ret < 0) CAGE_THROW_ERROR(Exception, "toString failed"); s[ret] = 0; return ret; } \
		void fromString(const char *s, TYPE &value) \
		{ return genericScan(s, value); }
		GCHL_GENERATE(sint8, %hhd);
		GCHL_GENERATE(sint16, %hd);
		GCHL_GENERATE(sint32, %d);
		GCHL_GENERATE(uint8, %hhu);
		GCHL_GENERATE(uint16, %hu);
		GCHL_GENERATE(uint32, %u);
#ifdef CAGE_SYSTEM_WINDOWS
		GCHL_GENERATE(sint64, %lld);
		GCHL_GENERATE(uint64, %llu);
#else
		GCHL_GENERATE(sint64, %ld);
		GCHL_GENERATE(uint64, %lu);
#endif
		GCHL_GENERATE(float, %f);
		GCHL_GENERATE(double, %lf);
#undef GCHL_GENERATE

		uint32 toString(char *s, bool value)
		{
			const char *src = value ? "true" : "false";
			std::strcpy(s, src);
			return numeric_cast<uint32>(strlen(s));
		}

		void fromString(const char *s, bool &value)
		{
			string l = string(s).toLower();
			if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off" || l == "0")
			{
				value = false;
				return;
			}
			if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on" || l == "1")
			{
				value = true;
				return;
			}
			CAGE_THROW_ERROR(Exception, "invalid value");
		}

		uint32 toString(char *dst, uint32 dstLen, const char *src)
		{
			auto l = std::strlen(src);
			if (l > dstLen)
				CAGE_THROW_ERROR(Exception, "string truncation");
			std::memcpy(dst, src, l);
			dst[l] = 0;
			return numeric_cast<uint32>(l);
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
					CAGE_THROW_ERROR(Exception, "string truncation");
				std::memmove(data + pos + withLen, data + pos + whatLen, current - pos - whatLen);
				std::memcpy(data + pos, with, withLen);
				current += withLen - whatLen;
				pos += withLen - whatLen + 1;
			}
		}

		void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right)
		{
			CAGE_ASSERT(isOrdered(what, whatLen));
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
					std::memmove(data, data + p, current);
			}
		}

		void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen)
		{
			CAGE_ASSERT(retLen == 0);
			CAGE_ASSERT(isOrdered(what, whatLen));
			if (whatLen == 0)
				return;
			for (uint32 i = 0; i < current; i++)
			{
				if (stringContains(what, whatLen, data[i]))
				{
					std::memcpy(ret, data, i);
					std::memmove(data, data + i + 1, current - i - 1);
					retLen = i;
					current -= i + 1;
					return;
				}
			}
			std::memcpy(ret, data, current);
			std::swap(current, retLen);
		}

		uint32 stringToUpper(char *dst, uint32 dstLen, const char *src, uint32 srcLen)
		{
			const char *e = src + srcLen;
			while (src != e)
				*dst++ = std::toupper(*src++);
			return srcLen;
		}

		uint32 stringToLower(char *dst, uint32 dstLen, const char *src, uint32 srcLen)
		{
			const char *e = src + srcLen;
			while (src != e)
				*dst++ = std::tolower(*src++);
			return srcLen;
		}

		uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset)
		{
			if (whatLen == 0 || offset + whatLen > current)
				return m;
			uint32 end = current - whatLen + 1;
			for (uint32 i = offset; i < end; i++)
				if (std::memcmp(data + i, what, whatLen) == 0)
					return i;
			return m;
		}

		void stringEncodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength)
		{
			char tmp[4096];
			uint32 len = encodeUrlBase(tmp, dataIn, currentIn);
			if (len > maxLength)
				CAGE_THROW_ERROR(Exception, "string truncation");
			std::memcpy(dataOut, tmp, len);
			currentOut = len;
		}

		void stringDecodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength)
		{
			char tmp[4096];
			uint32 len = decodeUrlBase(tmp, dataIn, currentIn);
			if (len > maxLength)
				CAGE_THROW_ERROR(Exception, "string truncation");
			std::memcpy(dataOut, tmp, len);
			currentOut = len;
		}

		bool stringIsDigitsOnly(const char *data, uint32 dataLen)
		{
			for (char c : PointerRange<const char>(data, data + dataLen))
			{
				if (c < '0' || c > '9')
					return false;
			}
			return true;
		}

		bool stringIsInteger(const char *data, uint32 dataLen, bool allowSign)
		{
			if (dataLen == 0)
				return false;
			if (allowSign && (data[0] == '-' || data[0] == '+'))
				return stringIsDigitsOnly(data + 1, dataLen - 1);
			else
				return stringIsDigitsOnly(data, dataLen);
		}

		bool stringIsReal(const char *data, uint32 dataLen)
		{
			if (dataLen == 0)
				return false;
			uint32 d = stringFind(data, dataLen, ".", 1, 0);
			if (d == m)
				return stringIsInteger(data, dataLen, true);
			return stringIsInteger(data, d, true) && stringIsDigitsOnly(data + d + 1, dataLen - d - 1);
		}

		bool stringIsBool(const char *data, uint32 dataLen)
		{
			if (dataLen > 10)
				return false;
			string l = string(data, dataLen).toLower();
			if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off")
				return true;
			if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on")
				return true;
			return false;
		}

		bool stringIsPattern(const char *data, uint32 dataLen, const char *prefix, uint32 prefixLen, const char *infix, uint32 infixLen, const char *suffix, uint32 suffixLen)
		{
			if (dataLen < prefixLen + infixLen + suffixLen)
				return false;
			if (std::memcmp(data, prefix, prefixLen) != 0)
				return false;
			if (std::memcmp(data + dataLen - suffixLen, suffix, suffixLen) != 0)
				return false;
			if (infixLen == 0)
				return true;
			uint32 pos = stringFind(data, dataLen, infix, infixLen, prefixLen);
			return pos != m && pos <= dataLen - infixLen - suffixLen;
		}

		int stringComparison(const char *ad, uint32 al, const char *bd, uint32 bl) noexcept
		{
			uint32 l = al < bl ? al : bl;
			int c = std::memcmp(ad, bd, l);
			if (c == 0)
				return al == bl ? 0 : al < bl ? -1 : 1;
			return c;
		}
	}
}
