#include <cage-core/utf.h>
#include <cage-core/pointerRangeHolder.h>

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
#define TRY_END catch (const std::exception &e) { CAGE_LOG_THROW(e.what()); CAGE_THROW_ERROR(InvalidUtfString, "string with invalid utf encoding"); }

namespace cage
{
	bool utfValid(PointerRange<const char> buffer)
	{
		TRY_BEGIN{
		return utf8::is_valid(buffer.begin(), buffer.end());
		}TRY_END
	}

	bool utfValid(const string &str)
	{
		return utfValid({ str.c_str(), str.c_str() + str.length() });
	}

	bool utfValid(const char *str)
	{
		return utfValid({ str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	uint32 utf32Length(PointerRange<const char> buffer)
	{
		TRY_BEGIN{
		return numeric_cast<uint32>(utf8::distance(buffer.begin(), buffer.end()));
		}TRY_END
	}

	uint32 utf32Length(const string &str)
	{
		return utf32Length({ str.c_str(), str.c_str() + str.length() });
	}

	uint32 utf32Length(const char *str)
	{
		return utf32Length({ str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	uint32 utf8Length(PointerRange<const uint32> buffer)
	{
		TRY_BEGIN{
		const uint32 *b = buffer.begin();
		const uint32 *e = buffer.end();
		char tmp[7];
		uint32 result = 0;
		while (b != e)
		{
			char *d = utf8::utf32to8(b, b + 1, tmp);
			result += numeric_cast<uint32>(d - tmp);
			b++;
		}
		return result;
		}TRY_END
	}

	Holder<PointerRange<uint32>> utf8to32(PointerRange<const char> inBuffer)
	{
		PointerRangeHolder<uint32> result;
		result.resize(utf32Length(inBuffer));
		TRY_BEGIN{
		uint32 *end = utf8::utf8to32(inBuffer.begin(), inBuffer.end(), result.data());
		CAGE_ASSERT(end == result.data() + result.size());
		return result;
		}TRY_END
	}

	Holder<PointerRange<uint32>> utf8to32(const string &str)
	{
		return utf8to32({ str.c_str(), str.c_str() + str.length() });
	}

	Holder<PointerRange<uint32>> utf8to32(const char *str)
	{
		return utf8to32({ str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	void utf8to32(PointerRange<uint32> &outBuffer, PointerRange<const char> inBuffer)
	{
		CAGE_ASSERT(outBuffer.size() >= utf32Length(inBuffer));
		TRY_BEGIN{
		uint32 *end = utf8::utf8to32(inBuffer.begin(), inBuffer.end(), outBuffer.begin());
		outBuffer = { outBuffer.begin(), end };
		}TRY_END
	}

	void utf8to32(PointerRange<uint32> &outBuffer, const string &str)
	{
		utf8to32(outBuffer, { str.c_str(), str.c_str() + str.length() });
	}

	void utf8to32(PointerRange<uint32> &outBuffer, const char *str)
	{
		utf8to32(outBuffer, { str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	Holder<PointerRange<char>> utf32to8(PointerRange<const uint32> buffer)
	{
		PointerRangeHolder<char> result;
		result.resize(utf8Length(buffer));
		TRY_BEGIN{
		char *end = utf8::utf32to8(buffer.begin(), buffer.end(), result.data());
		CAGE_ASSERT(end == result.data() + result.size());
		return result;
		}TRY_END
	}

	void utf32to8(PointerRange<char> &outBuffer, PointerRange<const uint32> inBuffer)
	{
		CAGE_ASSERT(outBuffer.size() >= utf8Length(inBuffer));
		TRY_BEGIN{
		char *end = utf8::utf32to8(inBuffer.begin(), inBuffer.end(), outBuffer.begin());
		outBuffer = { outBuffer.begin(), end };
		}TRY_END
	}

	string utf32to8string(PointerRange<const uint32> inBuffer)
	{
		if (utf8Length(inBuffer) > string::MaxLength)
			CAGE_THROW_ERROR(Exception, "utf string too long");
		char buff[string::MaxLength];
		PointerRange<char> pr = { buff, buff + string::MaxLength - 1 };
		utf32to8(pr, inBuffer);
		return string(pr);
	}
}
