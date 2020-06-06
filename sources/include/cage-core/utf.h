#ifndef guard_utf_h_C5D02E44A78446C9AAB8A83E5B27BC60
#define guard_utf_h_C5D02E44A78446C9AAB8A83E5B27BC60

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API InvalidUtfString : public Exception
	{
		using Exception::Exception;
		virtual void log();
	};

	CAGE_CORE_API bool utfValid(PointerRange<const char> buffer);
	CAGE_CORE_API bool utfValid(const string &str);
	CAGE_CORE_API bool utfValid(const char *str);

	// returns number of utf32 characters the string would have after conversion
	CAGE_CORE_API uint32 utf32Length(PointerRange<const char> buffer);
	CAGE_CORE_API uint32 utf32Length(const string &str);
	CAGE_CORE_API uint32 utf32Length(const char *str);

	// returns number of bytes the string would have after converting to utf8
	CAGE_CORE_API uint32 utf8Length(PointerRange<const uint32> buffer);

	CAGE_CORE_API Holder<PointerRange<uint32>> utf8to32(PointerRange<const char> buffer);
	CAGE_CORE_API Holder<PointerRange<uint32>> utf8to32(const string &str);
	CAGE_CORE_API Holder<PointerRange<uint32>> utf8to32(const char *str);
	CAGE_CORE_API void utf8to32(PointerRange<uint32> &outBuffer, PointerRange<const char> inBuffer);
	CAGE_CORE_API void utf8to32(PointerRange<uint32> &outBuffer, const string &inStr);
	CAGE_CORE_API void utf8to32(PointerRange<uint32> &outBuffer, const char *inStr);

	CAGE_CORE_API Holder<PointerRange<char>> utf32to8(PointerRange<const uint32> buffer);
	CAGE_CORE_API void utf32to8(PointerRange<char> &outBuffer, PointerRange<const uint32> inBuffer);
	CAGE_CORE_API string utf32to8string(PointerRange<const uint32> str);
}

#endif // guard_utf_h_C5D02E44A78446C9AAB8A83E5B27BC60
