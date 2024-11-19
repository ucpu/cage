#ifndef guard_unicode_h_fg45jhftg45zj4sdas
#define guard_unicode_h_fg45jhftg45zj4sdas

#include <cage-core/core.h>

namespace cage
{
	CAGE_CORE_API bool utfValid(PointerRange<const char> buffer);
	CAGE_CORE_API bool utfValid(const String &str);
	CAGE_CORE_API bool utfValid(const char *str);

	// returns number of utf32 characters the string would have after conversion
	CAGE_CORE_API uint32 utf32Length(PointerRange<const char> buffer);
	CAGE_CORE_API uint32 utf32Length(const String &str);
	CAGE_CORE_API uint32 utf32Length(const char *str);

	// returns number of bytes the string would have after converting to utf8
	CAGE_CORE_API uint32 utf8Length(PointerRange<const uint32> buffer);

	CAGE_CORE_API Holder<PointerRange<uint32>> utf8to32(PointerRange<const char> buffer);
	CAGE_CORE_API Holder<PointerRange<uint32>> utf8to32(const String &str);
	CAGE_CORE_API Holder<PointerRange<uint32>> utf8to32(const char *str);
	CAGE_CORE_API void utf8to32(PointerRange<uint32> &outBuffer, PointerRange<const char> inBuffer);
	CAGE_CORE_API void utf8to32(PointerRange<uint32> &outBuffer, const String &inStr);
	CAGE_CORE_API void utf8to32(PointerRange<uint32> &outBuffer, const char *inStr);

	CAGE_CORE_API Holder<PointerRange<char>> utf32to8(PointerRange<const uint32> buffer);
	CAGE_CORE_API void utf32to8(PointerRange<char> &outBuffer, PointerRange<const uint32> inBuffer);
	CAGE_CORE_API String utf32to8string(PointerRange<const uint32> str);

	enum class UnicodeTransformEnum : uint32
	{
		None = 0,
		Validate, // replaces invalid sequences
		CanonicalComposition, // NFC (canonical preserves exact meaning of the text, but removes distinct encodings)
		CanonicalDecomposition, // NFD
		CompatibilityComposition, // NFKC (compatibility removes small visual distinctions, possibly changing meaning)
		CompatibilityDecomposition, // NFKD
		Lowercase,
		Uppercase,
		Titlecase,
		Casefold, // used for caseless string matching
		Unaccent,
		FuzzyMatching, // casefold, unaccent, removes duplicates
	};

	struct CAGE_CORE_API UnicodeTransformConfig
	{
		UnicodeTransformEnum transform = UnicodeTransformEnum::Validate;
		const char *locale = nullptr; // optional, eg. en_US
	};

	CAGE_CORE_API Holder<PointerRange<char>> unicodeTransform(PointerRange<const char> buffer, const UnicodeTransformConfig &cfg);
	CAGE_CORE_API String unicodeTransformString(PointerRange<const char> buffer, const UnicodeTransformConfig &cfg);
}

#endif // guard_unicode_h_fg45jhftg45zj4sdas
