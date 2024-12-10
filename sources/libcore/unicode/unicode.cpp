#include <algorithm>

#include <uni_algo/case.h>
#include <uni_algo/conv.h>
#include <uni_algo/norm.h>

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
	}

	Holder<PointerRange<char>> unicodeTransform(PointerRange<const char> buffer, const UnicodeTransformConfig &cfg)
	{
		const std::string_view v = view(buffer);
		const una::locale l = una::locale(cfg.locale ? cfg.locale : "");
		switch (cfg.transform)
		{
			case UnicodeTransformEnum::None:
				return PointerRangeHolder<char>(buffer);
			case UnicodeTransformEnum::Validate:
			{
				const auto t = una::utf32to8u(una::utf8to32u(v));
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::CanonicalComposition:
			{
				const auto t = una::norm::to_nfc_utf8(v);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::CanonicalDecomposition:
			{
				const auto t = una::norm::to_nfd_utf8(v);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::CompatibilityComposition:
			{
				const auto t = una::norm::to_nfkc_utf8(v);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::CompatibilityDecomposition:
			{
				const auto t = una::norm::to_nfkd_utf8(v);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::Lowercase:
			{
				const auto t = una::cases::to_lowercase_utf8(v, l);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::Uppercase:
			{
				const auto t = una::cases::to_uppercase_utf8(v, l);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::Titlecase:
			{
				const auto t = una::cases::to_titlecase_utf8(v, l);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::Casefold:
			{
				const auto t = una::cases::to_casefold_utf8(v);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::Unaccent:
			{
				const auto t = una::norm::to_unaccent_utf8(v);
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			case UnicodeTransformEnum::FuzzyMatching:
			{
				auto t = una::norm::to_unaccent_utf8(una::cases::to_casefold_utf8(v));
				t.erase(std::unique(t.begin(), t.end()), t.end());
				return PointerRangeHolder<char>(t.begin(), t.end());
			}
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid UnicodeTransformEnum value");
		}
	}

	String unicodeTransformString(PointerRange<const char> buffer, const UnicodeTransformConfig &cfg)
	{
		const auto t = unicodeTransform(buffer, cfg);
		return String(PointerRange<const char>((const char *)t.data(), (const char *)t.data() + t.size()));
	}
}
