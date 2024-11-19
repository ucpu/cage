#include <cstring> // std::strlen
#include <string_view>

#include "uni_algo/conv.h"

#include <cage-core/pointerRangeHolder.h>
#include <cage-core/unicode.h>

namespace cage
{
	namespace
	{
		std::string_view view(PointerRange<const char> buffer)
		{
			return std::string_view(buffer.begin(), buffer.end());
		}

		std::u32string_view view(PointerRange<const uint32> buffer)
		{
			return std::u32string_view((const char32_t *)buffer.begin(), (const char32_t *)buffer.end());
		}
	}

	bool utfValid(PointerRange<const char> buffer)
	{
		return una::is_valid_utf8(view(buffer));
	}

	bool utfValid(const String &str)
	{
		return utfValid({ str.c_str(), str.c_str() + str.length() });
	}

	bool utfValid(const char *str)
	{
		return utfValid({ str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	uint32 utf32Length(PointerRange<const char> buffer)
	{
		return una::utf8to32u(view(buffer)).length();
	}

	uint32 utf32Length(const String &str)
	{
		return utf32Length({ str.c_str(), str.c_str() + str.length() });
	}

	uint32 utf32Length(const char *str)
	{
		return utf32Length({ str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	uint32 utf8Length(PointerRange<const uint32> buffer)
	{
		return una::utf32to8u(view(buffer)).length();
	}

	Holder<PointerRange<uint32>> utf8to32(PointerRange<const char> buffer)
	{
		const auto t = una::utf8to32u(view(buffer));
		return PointerRangeHolder<uint32>(t.begin(), t.end());
	}

	Holder<PointerRange<uint32>> utf8to32(const String &str)
	{
		return utf8to32({ str.c_str(), str.c_str() + str.length() });
	}

	Holder<PointerRange<uint32>> utf8to32(const char *str)
	{
		return utf8to32({ str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	void utf8to32(PointerRange<uint32> &outBuffer, PointerRange<const char> inBuffer)
	{
		const auto t = una::utf8to32u(view(inBuffer));
		CAGE_ASSERT(outBuffer.size() >= t.size());
		detail::memcpy(outBuffer.data(), t.data(), t.size() * sizeof(uint32));
		outBuffer = PointerRange<uint32>(outBuffer.begin(), outBuffer.begin() + t.size());
	}

	void utf8to32(PointerRange<uint32> &outBuffer, const String &str)
	{
		utf8to32(outBuffer, { str.c_str(), str.c_str() + str.length() });
	}

	void utf8to32(PointerRange<uint32> &outBuffer, const char *str)
	{
		utf8to32(outBuffer, { str, str + numeric_cast<uint32>(std::strlen(str)) });
	}

	Holder<PointerRange<char>> utf32to8(PointerRange<const uint32> buffer)
	{
		const auto t = una::utf32to8u(view(buffer));
		return PointerRangeHolder<char>(t.begin(), t.end());
	}

	void utf32to8(PointerRange<char> &outBuffer, PointerRange<const uint32> inBuffer)
	{
		const auto t = una::utf32to8u(view(inBuffer));
		CAGE_ASSERT(outBuffer.size() >= t.size());
		detail::memcpy(outBuffer.data(), t.data(), t.size() * sizeof(char));
		outBuffer = PointerRange<char>(outBuffer.begin(), outBuffer.begin() + t.size());
	}

	String utf32to8string(PointerRange<const uint32> buffer)
	{
		const auto t = una::utf32to8u(view(buffer));
		return String(PointerRange<const char>((const char *)t.data(), (const char *)t.data() + t.size()));
	}
}
