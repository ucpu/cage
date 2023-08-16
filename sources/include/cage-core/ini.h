#ifndef guard_ini_h_c866b123_b27e_4758_ab8e_702ef8f315de_
#define guard_ini_h_c866b123_b27e_4758_ab8e_702ef8f315de_

#include <cage-core/core.h>

/*
	rules for loading from file:
	char '#' denotes a comment (to end of line)
	empty lines (or lines containing only white chars ' ' and '\t') are skipped
	chars '\\', '\'' and '"' have no special meaning
	section names, item names and values are trimmed from both sides
	section names and item names may not contain any of '#', '=', '[' or ']'
	values may not contain '#'
	duplicate sections or duplicate items inside same section are invalid
	items before first section are invalid
	empty sections "[]" are converted to numerical sequences (beware of duplicities)
	empty items "value" or "=value" are converted to numerical sequences (beware of duplicities and values containing '=')
*/

/*
	rules for parsing command line parameters:
	strings starting with one dash (-o) are parsed as short options and may be grouped together (-abc are three options)
	strings starting with two dashes (--option) are parsed as long options and cannot be grouped
	string containing only two dashes (--) is parsed as an end of options and all remaining strings are treated as positional arguments
	strings that do not start with a dash (value) are added as arguments to last preceding option, if any, and to positional arguments otherwise
	strings -o and --o are considered same option
	last option is used when multiple same options exists
	options (short and long) without any arguments are automatically assigned single argument with value true
	option names are converted to section names (without dashes), their ordering is NOT preserved
	arguments are given numerical ids (items) and their ordering is preserved
	positional arguments are put into section [--]
*/

namespace cage
{
	class CAGE_CORE_API Ini : private Immovable
	{
	public:
		void clear();
		void merge(const Ini *source); // items in this are overridden by items in source
		void parseCmd(uint32 argc, const char *const args[]); // clears this before parsing
		void importBuffer(PointerRange<const char> buffer); // clears this before loading
		void importFile(const String &filename); // clears this before loading
		Holder<PointerRange<char>> exportBuffer() const;
		void exportFile(const String &filename) const;

		uint32 sectionsCount() const;
		String section(uint32 section) const;
		bool sectionExists(const String &section) const;
		Holder<PointerRange<String>> sections() const;
		void sectionRemove(const String &section);
		uint32 itemsCount(const String &section) const;
		String item(const String &section, uint32 item) const;
		bool itemExists(const String &section, const String &item) const;
		Holder<PointerRange<String>> items(const String &section) const;
		Holder<PointerRange<String>> values(const String &section) const;
		void itemRemove(const String &section, const String &item);

		String get(const String &section, const String &item) const;
		void set(const String &section, const String &item, const String &value);

		void markUsed(const String &section, const String &item);
		void markUnused(const String &section, const String &item);
		bool isUsed(const String &section, const String &item) const;
		bool anyUnused(String &section, String &item) const;
		bool anyUnused(String &section, String &item, String &value) const;
		void checkUnused() const;
		void logHelp() const; // log help based on all cmd* methods so far
		void checkUnusedWithHelp() const; // logs help and rethrows the exception, if any

#define GCHL_GENERATE(TYPE, NAME, DEF) \
	void set##NAME(const String &section, const String &item, const TYPE &value); \
	TYPE get##NAME(const String &section, const String &item, const TYPE &defaul = DEF) const; \
	TYPE cmd##NAME(char shortName, const String &longName, const TYPE &defaul); \
	TYPE cmd##NAME(char shortName, const String &longName);
		GCHL_GENERATE(bool, Bool, false);
		GCHL_GENERATE(sint32, Sint32, 0);
		GCHL_GENERATE(uint32, Uint32, 0);
		GCHL_GENERATE(sint64, Sint64, 0);
		GCHL_GENERATE(uint64, Uint64, 0);
		GCHL_GENERATE(float, Float, 0);
		GCHL_GENERATE(double, Double, 0);
		GCHL_GENERATE(String, String, "");
#undef GCHL_GENERATE

		Holder<PointerRange<String>> cmdArray(char shortName, const String &longName);
	};

	CAGE_CORE_API Holder<Ini> newIni();
}

#endif // guard_ini_h_c866b123_b27e_4758_ab8e_702ef8f315de_
