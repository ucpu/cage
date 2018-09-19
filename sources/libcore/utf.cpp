#define CAGE_EXPORT
#include <cage-core/core.h>
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

#define TRY_BEGIN try
#define TRY_END catch (const std::exception &) { CAGE_THROW_ERROR(utf8Exception, "Invalid utf-8 string"); }

namespace cage
{
	bool valid(const char *str)
	{
		return valid(str, numeric_cast<uint32>(detail::strlen(str)));
	}

	bool valid(const char *str, uint32 bytes)
	{
		TRY_BEGIN{
		return utf8::is_valid(str, str + bytes);
		}TRY_END
	}

	bool valid(const string &str)
	{
		return valid(str.c_str(), str.length());
	}

	uint32 countCharacters(const char *str)
	{
		return countCharacters(str, numeric_cast<uint32>(detail::strlen(str)));
	}

	uint32 countCharacters(const char *str, uint32 bytes)
	{
		TRY_BEGIN{
		return numeric_cast<uint32>(utf8::distance(str, str + bytes));
		}TRY_END
	}

	uint32 countCharacters(const string &str)
	{
		return countCharacters(str.c_str(), str.length());
	}

	void convert8to32(uint32 *outBuffer, uint32 &outCount, const char *inStr)
	{
		convert8to32(outBuffer, outCount, inStr, numeric_cast<uint32>(detail::strlen(inStr)));
	}

	void convert8to32(uint32 *outBuffer, uint32 &outCount, const char *inStr, uint32 inBytes)
	{
		TRY_BEGIN{
		CAGE_ASSERT_RUNTIME(outCount >= countCharacters(inStr, inBytes), outCount, countCharacters(inStr, inBytes), inStr, inBytes);
		uint32 *end = utf8::utf8to32(inStr, inStr + inBytes, outBuffer);
		outCount = numeric_cast<uint32>(end - outBuffer);
		}TRY_END
	}

	void convert8to32(uint32 *outBuffer, uint32 &outCount, const string &inStr)
	{
		convert8to32(outBuffer, outCount, inStr.c_str(), inStr.length());
	}

	void convert32to8(char *outBuffer, uint32 &outBytes, const uint32 *inBuffer, uint32 inCount)
	{
		TRY_BEGIN{
		char *end = utf8::utf32to8(inBuffer, inBuffer + inCount, outBuffer);
		CAGE_ASSERT_RUNTIME(numeric_cast<uint32>(end - outBuffer) <= outBytes, end - outBuffer, outBytes);
		outBytes = numeric_cast<uint32>(end - outBuffer);
		}TRY_END
	}

	string convert32to8(const uint32 *inBuffer, uint32 inCount)
	{
		char buff[string::MaxLength];
		uint32 len = string::MaxLength;
		convert32to8(buff, len, inBuffer, inCount);
		return string(buff, len);
	}
}