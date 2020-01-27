#ifndef guard_utf_h_C5D02E44A78446C9AAB8A83E5B27BC60
#define guard_utf_h_C5D02E44A78446C9AAB8A83E5B27BC60

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API InvalidUtfString : public Exception
	{
		explicit InvalidUtfString(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept;
		virtual void log();
	};

	CAGE_CORE_API bool valid(const char *str);
	CAGE_CORE_API bool valid(const char *str, uint32 bytes);
	CAGE_CORE_API bool valid(const string &str);

	// returns number of utf32 characters
	CAGE_CORE_API uint32 countCharacters(const char *str);
	CAGE_CORE_API uint32 countCharacters(const char *str, uint32 bytes);
	CAGE_CORE_API uint32 countCharacters(const string &str);

	CAGE_CORE_API void convert8to32(uint32 *outBuffer, uint32 &outCount, const char *inStr);
	CAGE_CORE_API void convert8to32(uint32 *outBuffer, uint32 &outCount, const char *inStr, uint32 inBytes);
	CAGE_CORE_API void convert8to32(uint32 *outBuffer, uint32 &outCount, const string &inStr);

	CAGE_CORE_API void convert32to8(char *outBuffer, uint32 &outBytes, const uint32 *inBuffer, uint32 inCount);
	CAGE_CORE_API string convert32to8(const uint32 *inBuffer, uint32 inCount);
}

#endif // guard_utf_h_C5D02E44A78446C9AAB8A83E5B27BC60
