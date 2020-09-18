#ifndef guard_string_h_sdrgh4s6ert4gzh6r5t4sedg48s9
#define guard_string_h_sdrgh4s6ert4gzh6r5t4sedg48s9

#include "core.h"

namespace cage
{
	namespace privat
	{
		CAGE_CORE_API void stringReplace(char *data, uint32 &current, uint32 maxLength, const char *what, uint32 whatLen, const char *with, uint32 withLen);
		CAGE_CORE_API void stringTrim(char *data, uint32 &current, const char *what, uint32 whatLen, bool left, bool right);
		CAGE_CORE_API void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, const char *what, uint32 whatLen);
		CAGE_CORE_API uint32 stringToUpper(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
		CAGE_CORE_API uint32 stringToLower(char *dst, uint32 dstLen, const char *src, uint32 srcLen);
		CAGE_CORE_API uint32 stringFind(const char *data, uint32 current, const char *what, uint32 whatLen, uint32 offset);
		CAGE_CORE_API void stringEncodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_CORE_API void stringDecodeUrl(const char *dataIn, uint32 currentIn, char *dataOut, uint32 &currentOut, uint32 maxLength);
		CAGE_CORE_API bool stringIsDigitsOnly(const char *data, uint32 dataLen);
		CAGE_CORE_API bool stringIsInteger(const char *data, uint32 dataLen);
		CAGE_CORE_API bool stringIsReal(const char *data, uint32 dataLen);
		CAGE_CORE_API bool stringIsBool(const char *data, uint32 dataLen);
		CAGE_CORE_API bool stringIsPattern(const char *data, uint32 dataLen, const char *prefix, uint32 prefixLen, const char *infix, uint32 infixLen, const char *suffix, uint32 suffixLen);
	}

	namespace detail
	{
		template<uint32 N>
		StringBase<N> StringBase<N>::reverse() const
		{
			StringBase ret = *this;
			for (uint32 i = 0; i < current; i++)
				ret.data[current - i - 1] = data[i];
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::subString(uint32 start, uint32 length) const
		{
			if (start >= current)
				return "";
			uint32 len = length;
			if (length == m || start + length > current)
				len = current - start;
			return StringBase(data + start, len);
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::replace(const StringBase &what, const StringBase &with) const
		{
			StringBase ret = *this;
			privat::stringReplace(ret.data, ret.current, N, what.data, what.current, with.data, with.current);
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::replace(uint32 start, uint32 length, const StringBase &with) const
		{
			return subString(0, start) + with + subString(start + length, m);
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::remove(uint32 start, uint32 length) const
		{
			return subString(0, start) + subString(start + length, m);
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::insert(uint32 start, const StringBase &what) const
		{
			return subString(0, start) + what + subString(start, m);
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::trim(bool left, bool right, const StringBase &trimChars) const
		{
			StringBase ret = *this;
			privat::stringTrim(ret.data, ret.current, trimChars.data, trimChars.current, left, right);
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::split(const StringBase &splitChars)
		{
			StringBase ret;
			privat::stringSplit(data, current, ret.data, ret.current, splitChars.data, splitChars.current);
			data[current] = 0;
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::fill(uint32 size, char c) const
		{
			StringBase cc(&c, 1);
			StringBase ret = *this;
			while (ret.length() < size)
				ret += cc;
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		uint32 StringBase<N>::find(const StringBase &other, uint32 offset) const
		{
			return privat::stringFind(data, current, other.data, other.current, offset);
		}

		template<uint32 N>
		uint32 StringBase<N>::find(char other, uint32 offset) const
		{
			return privat::stringFind(data, current, &other, 1, offset);
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::encodeUrl() const
		{
			StringBase ret;
			privat::stringEncodeUrl(data, current, ret.data, ret.current, N);
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::decodeUrl() const
		{
			StringBase ret;
			privat::stringDecodeUrl(data, current, ret.data, ret.current, N);
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::toUpper() const
		{
			StringBase ret;
			ret.current = privat::stringToUpper(ret.data, N, data, current);
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		StringBase<N> StringBase<N>::toLower() const
		{
			StringBase ret;
			ret.current = privat::stringToLower(ret.data, N, data, current);
			ret.data[ret.current] = 0;
			return ret;
		}

		template<uint32 N>
		float StringBase<N>::toFloat() const
		{
			float i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		double StringBase<N>::toDouble() const
		{
			double i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		sint32 StringBase<N>::toSint32() const
		{
			sint32 i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		uint32 StringBase<N>::toUint32() const
		{
			uint32 i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		sint64 StringBase<N>::toSint64() const
		{
			sint64 i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		uint64 StringBase<N>::toUint64() const
		{
			uint64 i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		bool StringBase<N>::toBool() const
		{
			bool i;
			privat::fromString(data, current, i);
			return i;
		}

		template<uint32 N>
		bool StringBase<N>::isPattern(const StringBase &prefix, const StringBase &infix, const StringBase &suffix) const
		{
			return privat::stringIsPattern(data, current, prefix.data, prefix.current, infix.data, infix.current, suffix.data, suffix.current);
		}

		template<uint32 N>
		bool StringBase<N>::isDigitsOnly() const noexcept
		{
			return privat::stringIsDigitsOnly(data, current);
		}

		template<uint32 N>
		bool StringBase<N>::isInteger() const noexcept
		{
			return privat::stringIsInteger(data, current);
		}

		template<uint32 N>
		bool StringBase<N>::isReal() const noexcept
		{
			return privat::stringIsReal(data, current);
		}

		template<uint32 N>
		bool StringBase<N>::isBool() const noexcept
		{
			return privat::stringIsBool(data, current);
		}

		template<uint32 N>
		struct StringComparatorFastBase
		{
			bool operator () (const StringBase<N> &a, const StringBase<N> &b) const noexcept
			{
				if (a.length() == b.length())
					return detail::memcmp(a.begin(), b.begin(), a.length()) < 0;
				return a.length() < b.length();
			}
		};
	}

	using stringComparatorFast = detail::StringComparatorFastBase<995>;
}

#endif // guard_string_h_sdrgh4s6ert4gzh6r5t4sedg48s9
