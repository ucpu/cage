#include <cage-core/utf.h>

#ifdef CAGE_DEBUG
#include "utf8/checked.h"
#else
#include "utf8/unchecked.h"
namespace utf8
{
	using namespace unchecked;
}
#endif

#include <cstring>
#include <exception>

#define TRY_BEGIN try
#define TRY_END catch (const std::exception &e) { CAGE_THROW_ERROR(InvalidUtfString, e.what()); }

namespace cage
{
	InvalidUtfString::InvalidUtfString(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message) noexcept : Exception(file, line, function, severity, message)
	{};

	void InvalidUtfString::log()
	{
		::cage::privat::makeLog(file, line, function, severity, "exception", string() + "utf8 error: '" + message + "'", false, false);
	};

	bool utfValid(const char *str)
	{
		return utfValid(str, numeric_cast<uint32>(std::strlen(str)));
	}

	bool utfValid(const char *str, uint32 bytes)
	{
		TRY_BEGIN{
		return utf8::is_valid(str, str + bytes);
		}TRY_END
	}

	bool utfValid(const string &str)
	{
		return utfValid(str.c_str(), str.length());
	}

	uint32 utfLength(const char *str)
	{
		return utfLength(str, numeric_cast<uint32>(std::strlen(str)));
	}

	uint32 utfLength(const char *str, uint32 bytes)
	{
		TRY_BEGIN{
		return numeric_cast<uint32>(utf8::distance(str, str + bytes));
		}TRY_END
	}

	uint32 utfLength(const string &str)
	{
		return utfLength(str.c_str(), str.length());
	}

	void utf8to32(uint32 *outBuffer, uint32 &outCount, const char *inStr)
	{
		utf8to32(outBuffer, outCount, inStr, numeric_cast<uint32>(std::strlen(inStr)));
	}

	void utf8to32(uint32 *outBuffer, uint32 &outCount, const char *inStr, uint32 inBytes)
	{
		TRY_BEGIN{
		CAGE_ASSERT(outCount >= utfLength(inStr, inBytes));
		uint32 *end = utf8::utf8to32(inStr, inStr + inBytes, outBuffer);
		outCount = numeric_cast<uint32>(end - outBuffer);
		}TRY_END
	}

	void utf8to32(uint32 *outBuffer, uint32 &outCount, const string &inStr)
	{
		utf8to32(outBuffer, outCount, inStr.c_str(), inStr.length());
	}

	void utf32to8(char *outBuffer, uint32 &outBytes, const uint32 *inBuffer, uint32 inCount)
	{
		TRY_BEGIN{
		char *end = utf8::utf32to8(inBuffer, inBuffer + inCount, outBuffer);
		CAGE_ASSERT(numeric_cast<uint32>(end - outBuffer) <= outBytes);
		outBytes = numeric_cast<uint32>(end - outBuffer);
		}TRY_END
	}

	string utf32to8(const uint32 *inBuffer, uint32 inCount)
	{
		char buff[string::MaxLength];
		uint32 len = string::MaxLength;
		utf32to8(buff, len, inBuffer, inCount);
		return string(buff, len);
	}
}
