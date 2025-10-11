#ifndef guard_texts_h_B436B597745C461DAA266CE6FBBE10D1
#define guard_texts_h_B436B597745C461DAA266CE6FBBE10D1

#include <cage-core/core.h>

namespace cage
{
	using LanguageCode = detail::StringBase<10>;

	class CAGE_CORE_API Texts : private Immovable
	{
	public:
		void clear();
		Holder<Texts> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void set(uint32 id, const String &text, const LanguageCode &language);
		uint32 set(const String &name, const String &text, const LanguageCode &language);

		Holder<PointerRange<LanguageCode>> allLanguages() const;
		Holder<PointerRange<String>> allNames() const;
		Holder<PointerRange<uint32>> allIds() const;

		// returns empty string if not found
		String get(uint32 id, const LanguageCode &language) const;
	};

	CAGE_CORE_API Holder<Texts> newTexts();

	CAGE_CORE_API void textsSetLanguages(PointerRange<const LanguageCode> languages);
	CAGE_CORE_API void textsSetLanguages(const String &languages); // list of languages separated by ;
	CAGE_CORE_API Holder<PointerRange<LanguageCode>> textsGetLanguages();
	CAGE_CORE_API Holder<PointerRange<LanguageCode>> textsGetAvailableLanguages();
	CAGE_CORE_API void textsAdd(const Texts *txt);
	CAGE_CORE_API void textsRemove(const Texts *txt);
	CAGE_CORE_API String textsGet(uint32 id, String params = "");

	CAGE_CORE_API String textFormat(String format, const String &params);
}

#endif // guard_texts_h_B436B597745C461DAA266CE6FBBE10D1
