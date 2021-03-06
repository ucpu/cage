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

		void set(uint32 name, const string &text);
		void erase(uint32 name);

		string get(uint32 name) const;
		string format(uint32 name, PointerRange<const string> params) const;

		static string format(const string &format, PointerRange<const string> params);
	};

	CAGE_CORE_API Holder<TextPack> newTextPack();

	CAGE_CORE_API AssetScheme genAssetSchemeTextPack();
	constexpr uint32 AssetSchemeIndexTextPack = 2;
}

#endif // guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1
