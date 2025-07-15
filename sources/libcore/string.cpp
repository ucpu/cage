#include <algorithm>
#include <cctype> // std::isspace
#include <cerrno>
#include <charconv> // to_chars, from_chars
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <cage-core/macros.h>
#include <cage-core/string.h>

namespace cage
{
	static_assert((sizeof(cage::String) % 8) == 0, "size of string is not aligned");

	namespace detail
	{
		void *memset(void *ptr, int value, uintPtr num)
		{
			CAGE_ASSERT(ptr || num == 0);
			return std::memset(ptr, value, num);
		}

		void *memcpy(void *destination, const void *source, uintPtr num)
		{
			CAGE_ASSERT(destination); // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
			CAGE_ASSERT(source);
			return std::memcpy(destination, source, num);
		}

		void *memmove(void *destination, const void *source, uintPtr num)
		{
			CAGE_ASSERT(destination); // If either dest or src is an invalid or null pointer, the behavior is undefined, even if count is zero.
			CAGE_ASSERT(source);
			return std::memmove(destination, source, num);
		}

		int memcmp(const void *ptr1, const void *ptr2, uintPtr num)
		{
			CAGE_ASSERT(ptr1 || num == 0);
			CAGE_ASSERT(ptr2 || num == 0);
			return std::memcmp(ptr1, ptr2, num);
		}
	}

	namespace privat
	{
		namespace
		{
			bool isOrdered(const char *data, uint32 current)
			{
				// use adjacent_find to test that there are no duplicates
				return std::is_sorted(data, data + current) && std::adjacent_find(data, data + current) == data + current;
			}

			bool stringContains(const char *data, uint32 current, char what)
			{
				CAGE_ASSERT(isOrdered(data, current));
				return std::binary_search(data, data + current, what);
			}

			uint32 encodeUrlBase(uint8 *pStart, const uint8 *pSrc, uint32 length)
			{
				static constexpr bool SAFE[256] = { //
					/*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
					/* 0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* 1 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* 2 */ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, // (, ), +, -, .
					/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

					/* 4 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, // _
					/* 6 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, // ~

					/* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

					/* C */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* D */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* E */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					/* F */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
				};
				static constexpr uint8 DEC2HEX[16 + 1] = "0123456789ABCDEF";
				uint8 *pEnd = pStart;
				const uint8 *const SRC_END = pSrc + length;
				for (; pSrc < SRC_END; ++pSrc)
				{
					if (SAFE[*pSrc])
						*pEnd++ = *pSrc;
					else
					{
						*pEnd++ = '%';
						*pEnd++ = DEC2HEX[(*pSrc) >> 4];
						*pEnd++ = DEC2HEX[(*pSrc) & 0x0F];
					}
				}
				return numeric_cast<uint32>(pEnd - pStart);
			}

			uint32 decodeUrlBase(uint8 *pStart, const uint8 *pSrc, uint32 length)
			{
				static constexpr sint8 HEX2DEC[256] = { //
					/*       0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
					/* 0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 1 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 2 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 3 */ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,

					/* 4 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 5 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 6 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 7 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

					/* 8 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* 9 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* A */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* B */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

					/* C */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* D */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* E */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
					/* F */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
				};
				const uint8 *const SRC_END = pSrc + length;
				const uint8 *const SRC_LAST_DEC = SRC_END - 2;
				uint8 *pEnd = pStart;
				while (pSrc < SRC_LAST_DEC)
				{
					if (*pSrc == '%')
					{
						uint8 dec1, dec2;
						if (uint8(-1) != (dec1 = HEX2DEC[*(pSrc + 1)]) && uint8(-1) != (dec2 = HEX2DEC[*(pSrc + 2)]))
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

#define GCHL_FROMSTRING(TYPE) \
	void fromString(const char *s, uint32 n, TYPE &value) \
	{ \
		const auto [p, ec] = std::from_chars(s, s + n, value); \
		if (p != s + n || ec != std::errc()) \
		{ \
			CAGE_LOG_THROW(Stringizer() + "input string: " + s); \
			CAGE_THROW_ERROR(Exception, "failed conversion of string to " CAGE_STRINGIZE(TYPE)); \
		} \
	}

#define GCHL_GENERATE(TYPE) \
	uint32 toString(char *s, uint32 n, TYPE value) \
	{ \
		const auto [p, ec] = std::to_chars(s, s + n, value); \
		if (ec != std::errc()) \
			CAGE_THROW_ERROR(Exception, "failed conversion of " CAGE_STRINGIZE(TYPE) " to string"); \
		*p = 0; \
		return numeric_cast<uint32>(p - s); \
	} \
	GCHL_FROMSTRING(TYPE)

		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
#ifdef CAGE_SYSTEM_MAC
		GCHL_GENERATE(std::size_t);
#endif
#undef GCHL_GENERATE

#ifdef CAGE_SYSTEM_MAC
		// macOS doesn't support from_chars for floating point yet
		void fromString(const char *s, uint32 n, float &value)
		{
			char buffer[256];
			if (n >= sizeof(buffer))
				CAGE_THROW_ERROR(Exception, "string too long for float conversion");
			std::memcpy(buffer, s, n);
			buffer[n] = '\0';
			char *endptr;
			errno = 0;
			value = std::strtof(buffer, &endptr);
			if (errno != 0 || endptr != buffer + n)
			{
				CAGE_LOG_THROW(Stringizer() + "input string: " + s);
				CAGE_THROW_ERROR(Exception, "failed conversion of string to float");
			}
		}

		void fromString(const char *s, uint32 n, double &value)
		{
			char buffer[256];
			if (n >= sizeof(buffer))
				CAGE_THROW_ERROR(Exception, "string too long for double conversion");
			std::memcpy(buffer, s, n);
			buffer[n] = '\0';
			char *endptr;
			errno = 0;
			value = std::strtod(buffer, &endptr);
			if (errno != 0 || endptr != buffer + n)
			{
				CAGE_LOG_THROW(Stringizer() + "input string: " + s);
				CAGE_THROW_ERROR(Exception, "failed conversion of string to double");
			}
		}

		uint32 toString(char *s, uint32 n, float value)
		{
			int result = std::snprintf(s, n, "%f", value);
			if (result < 0 || (uint32)result >= n)
				CAGE_THROW_ERROR(Exception, "failed conversion of float to string");
			// Remove trailing zeros
			char *p = s + result - 1;
			while (p > s && *p == '0') p--;
			if (*p == '.') p--;
			*(++p) = '\0';
			return numeric_cast<uint32>(p - s);
		}

		uint32 toString(char *s, uint32 n, double value)
		{
			int result = std::snprintf(s, n, "%f", value);
			if (result < 0 || (uint32)result >= n)
				CAGE_THROW_ERROR(Exception, "failed conversion of double to string");
			// Remove trailing zeros
			char *p = s + result - 1;
			while (p > s && *p == '0') p--;
			if (*p == '.') p--;
			*(++p) = '\0';
			return numeric_cast<uint32>(p - s);
		}
#else // CAGE_SYSTEM_MAC
#define GCHL_GENERATE(TYPE) \
	uint32 toString(char *s, uint32 n, TYPE value) \
	{ \
		const auto [p, ec] = std::to_chars(s, s + n, value, std::chars_format::fixed); \
		if (ec != std::errc()) \
			CAGE_THROW_ERROR(Exception, "failed conversion of " CAGE_STRINGIZE(TYPE) " to string"); \
		*p = 0; \
		return numeric_cast<uint32>(p - s); \
	} \
	GCHL_FROMSTRING(TYPE)

		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
#endif // CAGE_SYSTEM_MAC
#undef GCHL_GENERATE

		uint32 toString(char *s, uint32 n, bool value)
		{
			const char *src = value ? "true" : "false";
			std::strcpy(s, src);
			return numeric_cast<uint32>(strlen(s));
		}

		void fromString(const char *s, uint32 n, bool &value)
		{
			const String l = toLower(String(s));
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

		uint32 toStringImpl(char *s, uint32 n, const char *src)
		{
			const auto l = std::strlen(src);
			if (l > n)
				CAGE_THROW_ERROR(Exception, "string truncation");
			std::memcpy(s, src, l);
			s[l] = 0;
			return numeric_cast<uint32>(l);
		}

#define GCHL_GENERATE(TYPE) \
	void fromString(const PointerRange<const char> &str, TYPE &value) \
	{ \
		fromString(str.data(), numeric_cast<uint32>(str.size()), value); \
	}
		GCHL_GENERATE(sint8);
		GCHL_GENERATE(sint16);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(sint64);
		GCHL_GENERATE(uint8);
		GCHL_GENERATE(uint16);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(uint64);
		GCHL_GENERATE(float);
		GCHL_GENERATE(double);
		GCHL_GENERATE(bool);
#undef GCHL_GENERATE

		int stringComparisonImpl(const char *ad, uint32 al, const char *bd, uint32 bl)
		{
			const uint32 l = al < bl ? al : bl;
			const int c = std::memcmp(ad, bd, l);
			if (c == 0)
				return al == bl ? 0 : al < bl ? -1 : 1;
			return c;
		}

		void stringReplace(char *data, uint32 &current, uint32 maxLength, PointerRange<const char> what, PointerRange<const char> with)
		{
			if (what.size() == 0)
				return;
			uint32 pos = 0;
			while (true)
			{
				pos = stringFind({ data, data + current }, what, pos);
				if (pos == m)
					break;
				if (current + with.size() - what.size() > maxLength)
					CAGE_THROW_ERROR(Exception, "string truncation");
				std::memmove(data + pos + with.size(), data + pos + what.size(), current - pos - what.size());
				std::memcpy(data + pos, with.data(), with.size());
				sint32 move = numeric_cast<sint32>(with.size()) - numeric_cast<sint32>(what.size());
				current += move;
				pos += move + 1;
			}
			data[current] = 0;
		}

		void stringTrim(char *data, uint32 &current, PointerRange<const char> what, bool left, bool right)
		{
			CAGE_ASSERT(isOrdered(what.data(), numeric_cast<uint32>(what.size())));
			if (what.size() == 0)
				return;
			if (!left && !right)
				return;
			if (right)
			{
				uint32 p = 0;
				while (p < current && stringContains(what.data(), numeric_cast<uint32>(what.size()), data[current - p - 1]))
					p++;
				current -= p;
			}
			if (left)
			{
				uint32 p = 0;
				while (p < current && stringContains(what.data(), numeric_cast<uint32>(what.size()), data[p]))
					p++;
				current -= p;
				if (p > 0)
					std::memmove(data, data + p, current);
			}
			data[current] = 0;
		}

		void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, PointerRange<const char> what)
		{
			CAGE_ASSERT(retLen == 0);
			CAGE_ASSERT(isOrdered(what.data(), numeric_cast<uint32>(what.size())));
			if (what.size() == 0)
				return;
			for (uint32 i = 0; i < current; i++)
			{
				if (stringContains(what.data(), numeric_cast<uint32>(what.size()), data[i]))
				{
					std::memcpy(ret, data, i);
					std::memmove(data, data + i + 1, current - i - 1);
					retLen = i;
					current -= i + 1;
					data[current] = 0;
					ret[retLen] = 0;
					return;
				}
			}
			std::memcpy(ret, data, current);
			std::swap(current, retLen);
			data[current] = 0;
			ret[retLen] = 0;
		}

		uint32 stringFind(PointerRange<const char> data, PointerRange<const char> what, uint32 offset)
		{
			if (what.size() == 0 || offset + what.size() > data.size())
				return m;
			const uint32 end = numeric_cast<uint32>(data.size() - what.size() + 1);
			for (uint32 i = offset; i < end; i++)
				if (std::memcmp(data.data() + i, what.data(), what.size()) == 0)
					return i;
			return m;
		}

		void stringEncodeUrl(char *data, uint32 &current, uint32 maxLength, PointerRange<const char> what)
		{
			// todo this can buffer overflow
			uint8 tmp[4096];
			const uint32 len = encodeUrlBase(tmp, (uint8 *)what.data(), numeric_cast<uint32>(what.size()));
			if (len > maxLength)
				CAGE_THROW_ERROR(Exception, "string truncation");
			std::memcpy(data, tmp, len);
			current = len;
			data[current] = 0;
		}

		void stringDecodeUrl(char *data, uint32 &current, uint32 maxLength, PointerRange<const char> what)
		{
			CAGE_ASSERT(maxLength >= what.size());
			current = decodeUrlBase((uint8 *)data, (const uint8 *)what.data(), numeric_cast<uint32>(what.size()));
			data[current] = 0;
		}

		void stringToUpper(PointerRange<char> data)
		{
			for (char &c : data)
				c = std::toupper(c);
		}

		void stringToLower(PointerRange<char> data)
		{
			for (char &c : data)
				c = std::tolower(c);
		}

		bool stringIsPattern(PointerRange<const char> data, PointerRange<const char> prefix, PointerRange<const char> infix, PointerRange<const char> suffix)
		{
			if (data.size() < prefix.size() + infix.size() + suffix.size())
				return false;
			if (std::memcmp(data.data(), prefix.data(), prefix.size()) != 0)
				return false;
			if (std::memcmp(data.data() + data.size() - suffix.size(), suffix.data(), suffix.size()) != 0)
				return false;
			if (infix.size() == 0)
				return true;
			const uint32 pos = stringFind(data, infix, numeric_cast<uint32>(prefix.size()));
			return pos != m && pos <= data.size() - infix.size() - suffix.size();
		}

		bool stringIsDigitsOnly(PointerRange<const char> data)
		{
			for (char c : data)
			{
				if (c < '0' || c > '9')
					return false;
			}
			return true;
		}

		bool stringIsInteger(PointerRange<const char> data)
		{
			if (data.size() == 0)
				return false;
			if (data[0] == '-' && data.size() > 1)
				return stringIsDigitsOnly({ data.data() + 1, data.data() + data.size() - 1 });
			else
				return stringIsDigitsOnly(data);
		}

		bool stringIsReal(PointerRange<const char> data)
		{
			if (data.size() == 0)
				return false;
			const uint32 d = stringFind(data, ".", 0);
			if (d == m)
				return stringIsInteger(data);
			return stringIsInteger({ data.data(), data.data() + d }) && stringIsDigitsOnly({ data.data() + d + 1, data.end() });
		}

		bool stringIsBool(PointerRange<const char> data)
		{
			if (data.size() > 10)
				return false;
			const String l = toLower(String(data));
			if (l == "false" || l == "f" || l == "no" || l == "n" || l == "off")
				return true;
			if (l == "true" || l == "t" || l == "yes" || l == "y" || l == "on")
				return true;
			return false;
		}
	}

	namespace detail
	{
		namespace
		{
			constexpr bool digit(char c)
			{
				return c >= '0' && c <= '9';
			}

			constexpr uint64 tonum(PointerRange<const char> in, const char *&p)
			{
				CAGE_ASSERT(p >= in.begin() && p < in.end());
				CAGE_ASSERT(digit(*p));
				uint64 r = 0;
				while (p != in.end() && digit(*p))
				{
					r *= 10;
					r += *p++ - '0';
				}
				return r;
			}

			constexpr bool testtonum()
			{
				const char s[] = "abc123def";
				const char *p = s + 3;
				const uint64 n = tonum(s, p);
				return n == 123 && p == s + 6;
			}

			static_assert(testtonum());
		}

		int naturalComparison(PointerRange<const char> a, PointerRange<const char> b)
		{
			const char *i = a.begin();
			const char *j = b.begin();
			while (true)
			{
				if (i == a.end() && j == b.end())
					return 0;
				if (i == a.end())
					return -1;
				if (j == b.end())
					return 1;
				if (digit(*i) && digit(*j))
				{
					const uint64 ax = tonum(a, i);
					const uint64 bx = tonum(b, j);
					if (ax < bx)
						return -1;
					if (ax > bx)
						return 1;
				}
				if (*i < *j)
					return -1;
				if (*i > *j)
					return 1;
				i++;
				j++;
			}
		}
	}
}
