#ifndef guard_string_h_sdrgh4s6ert4gzh6r5t4sedg48s9
#define guard_string_h_sdrgh4s6ert4gzh6r5t4sedg48s9

#include "core.h"

namespace cage
{
	namespace privat
	{
		CAGE_CORE_API void stringReplace(char *data, uint32 &current, uint32 maxLength, PointerRange<const char> what, PointerRange<const char> with);
		CAGE_CORE_API void stringTrim(char *data, uint32 &current, PointerRange<const char> what, bool left, bool right);
		CAGE_CORE_API void stringSplit(char *data, uint32 &current, char *ret, uint32 &retLen, PointerRange<const char> what);
		CAGE_CORE_API uint32 stringFind(PointerRange<const char> data, PointerRange<const char> what, uint32 offset);
		CAGE_CORE_API void stringEncodeUrl(char *data, uint32 &current, uint32 maxLength, PointerRange<const char> what);
		CAGE_CORE_API void stringDecodeUrl(char *data, uint32 &current, uint32 maxLength, PointerRange<const char> what);
		CAGE_CORE_API void stringToUpper(PointerRange<char> data);
		CAGE_CORE_API void stringToLower(PointerRange<char> data);
		CAGE_CORE_API bool stringIsPattern(PointerRange<const char> data, PointerRange<const char> prefix, PointerRange<const char> infix, PointerRange<const char> suffix);
		CAGE_CORE_API bool stringIsDigitsOnly(PointerRange<const char> data);
		CAGE_CORE_API bool stringIsInteger(PointerRange<const char> data);
		CAGE_CORE_API bool stringIsReal(PointerRange<const char> data);
		CAGE_CORE_API bool stringIsBool(PointerRange<const char> data);
	}

	template<uint32 N>
	detail::StringBase<N> reverse(const detail::StringBase<N> &str)
	{
		detail::StringBase<N> ret;
		ret.rawLength() = str.length();
		for (uint32 i = 0; i < str.length(); i++)
			ret[str.length() - i - 1] = str[i];
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> subString(const detail::StringBase<N> &str, uint32 start, uint32 length)
	{
		if (start >= str.length())
			return "";
		uint32 len = length;
		if (length == m || start + length > str.length())
			len = str.length() - start;
		return detail::StringBase<N>({ str.c_str() + start, str.c_str() + start + len });
	}

	template<uint32 N>
	detail::StringBase<N> replace(const detail::StringBase<N> &str, PointerRange<const char> what, PointerRange<const char> with)
	{
		detail::StringBase<N> ret = str;
		privat::stringReplace(ret.rawData(), ret.rawLength(), N, what, with);
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> replace(const detail::StringBase<N> &str, uint32 start, uint32 length, PointerRange<const char> with)
	{
		return subString(str, 0, start) + detail::StringBase<N>(with) + subString(str, start + length, m);
	}

	template<uint32 N>
	detail::StringBase<N> remove(const detail::StringBase<N> &str, uint32 start, uint32 length)
	{
		return subString(str, 0, start) + subString(str, start + length, m);
	}

	template<uint32 N>
	detail::StringBase<N> insert(const detail::StringBase<N> &str, uint32 start, PointerRange<const char> what)
	{
		return subString(str, 0, start) + detail::StringBase<N>(what) + subString(str, start, m);
	}

	template<uint32 N>
	detail::StringBase<N> trim(const detail::StringBase<N> &str, bool left = true, bool right = true, PointerRange<const char> trimChars = "\t\n ")
	{
		detail::StringBase<N> ret = str;
		privat::stringTrim(ret.rawData(), ret.rawLength(), trimChars, left, right);
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> split(detail::StringBase<N> &str, PointerRange<const char> splitChars = "\t\n ")
	{
		detail::StringBase<N> ret;
		privat::stringSplit(str.rawData(), str.rawLength(), ret.rawData(), ret.rawLength(), splitChars);
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> fill(const detail::StringBase<N> &str, uint32 size, char c = ' ')
	{
		detail::StringBase<N> ret = str;
		while (ret.length() < size)
			ret += detail::StringBase<N>(c);
		return ret;
	}

	template<uint32 N>
	uint32 find(const detail::StringBase<N> &str, PointerRange<const char> what, uint32 offset = 0)
	{
		return privat::stringFind(str, what, offset);
	}

	template<uint32 N>
	uint32 find(const detail::StringBase<N> &str, char what, uint32 offset = 0)
	{
		return privat::stringFind(str, { &what, &what + 1 }, offset);
	}

	template<uint32 N>
	detail::StringBase<N> encodeUrl(const detail::StringBase<N> &str)
	{
		detail::StringBase<N> ret;
		privat::stringEncodeUrl(ret.rawData(), ret.rawLength(), N, str);
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> decodeUrl(const detail::StringBase<N> &str)
	{
		detail::StringBase<N> ret;
		privat::stringDecodeUrl(ret.rawData(), ret.rawLength(), N, str);
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> toUpper(const detail::StringBase<N> &str)
	{
		detail::StringBase<N> ret = str;
		privat::stringToUpper({ ret.rawData(), ret.rawData() + ret.length() });
		return ret;
	}

	template<uint32 N>
	detail::StringBase<N> toLower(const detail::StringBase<N> &str)
	{
		detail::StringBase<N> ret = str;
		privat::stringToLower({ ret.rawData(), ret.rawData() + ret.length() });
		return ret;
	}

	template<uint32 N>
	float toFloat(const detail::StringBase<N> &str)
	{
		float i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	double toDouble(const detail::StringBase<N> &str)
	{
		double i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	sint32 toSint32(const detail::StringBase<N> &str)
	{
		sint32 i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	uint32 toUint32(const detail::StringBase<N> &str)
	{
		uint32 i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	sint64 toSint64(const detail::StringBase<N> &str)
	{
		sint64 i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	uint64 toUint64(const detail::StringBase<N> &str)
	{
		uint64 i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	bool toBool(const detail::StringBase<N> &str)
	{
		bool i;
		privat::fromString(str, i);
		return i;
	}

	template<uint32 N>
	bool isPattern(const detail::StringBase<N> &str, PointerRange<const char> prefix, PointerRange<const char> infix, PointerRange<const char> suffix)
	{
		return privat::stringIsPattern(str, prefix, infix, suffix);
	}

	template<uint32 N>
	bool isDigitsOnly(const detail::StringBase<N> &str) noexcept
	{
		return privat::stringIsDigitsOnly(str);
	}

	template<uint32 N>
	bool isInteger(const detail::StringBase<N> &str) noexcept
	{
		return privat::stringIsInteger(str);
	}

	template<uint32 N>
	bool isReal(const detail::StringBase<N> &str) noexcept
	{
		return privat::stringIsReal(str);
	}

	template<uint32 N>
	bool isBool(const detail::StringBase<N> &str) noexcept
	{
		return privat::stringIsBool(str);
	}

	namespace detail
	{
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

		CAGE_CORE_API int naturalComparison(PointerRange<const char> a, PointerRange<const char> b);

		template<uint32 N>
		struct StringComparatorNaturalBase
		{
			bool operator () (const StringBase<N> &a, const StringBase<N> &b) const noexcept
			{
				return naturalComparison(a, b) < 0;
			}
		};
	}

	using StringComparatorFast = detail::StringComparatorFastBase<995>;
	using StringComparatorNatural = detail::StringComparatorNaturalBase<995>;
}

#endif // guard_string_h_sdrgh4s6ert4gzh6r5t4sedg48s9
