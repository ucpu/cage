#ifndef guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1
#define guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1

#include "core.h"

namespace cage
{
	class CAGE_CORE_API TextPack : private Immovable
	{
	public:
		void clear();
		Holder<TextPack> copy() const;

		void importBuffer(PointerRange<const char> buffer);
		Holder<PointerRange<char>> exportBuffer() const;

		void set(uint32 name, const String &text);
		void erase(uint32 name);

		String get(uint32 name) const;
		String format(uint32 name, PointerRange<const String> params) const;

		static String format(const String &format, PointerRange<const String> params);
	};

	CAGE_CORE_API Holder<TextPack> newTextPack();

	CAGE_CORE_API AssetScheme genAssetSchemeTextPack();
	constexpr uint32 AssetSchemeIndexTextPack = 2;

	CAGE_CORE_API String loadFormattedString(AssetManager *assets, uint32 asset, uint32 text, String params);
}

#endif // guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1
