#ifndef guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1
#define guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1

namespace cage
{
	class CAGE_API textPackClass : private immovable
	{
	public:
		void set(uint32 name, const string &text);
		void erase(uint32 name);
		void clear();

		string get(uint32 name) const;
		string format(uint32 name, uint32 paramCount, const string *paramValues) const;

		static string format(const string &format, uint32 paramCount, const string *paramValues);
	};

	CAGE_API holder<textPackClass> newTextPack();

	CAGE_API assetSchemeStruct genAssetSchemeTextPackage(const uint32 threadIndex);
	static const uint32 assetSchemeIndexTextPackage = 2;
}

#endif // guard_textPack_h_B436B597745C461DAA266CE6FBBE10D1
