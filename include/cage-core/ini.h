#ifndef guard_ini_h_c866b123_b27e_4758_ab8e_702ef8f315de_
#define guard_ini_h_c866b123_b27e_4758_ab8e_702ef8f315de_

#include "core.h"

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
	string containing only two dashes (--) is parsed as an end of options and all remaining string are treated as positional arguments
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
		uint32 sectionsCount() const;
		string section(uint32 section) const;
		bool sectionExists(const string &section) const;
		Holder<PointerRange<string>> sections() const;
		void sectionRemove(const string &section);
		uint32 itemsCount(const string &section) const;
		string item(const string &section, uint32 item) const;
		bool itemExists(const string &section, const string &item) const;
		Holder<PointerRange<string>> items(const string &section) const;
		Holder<PointerRange<string>> values(const string &section) const;
		void itemRemove(const string &section, const string &item);

		string get(const string &section, const string &item) const;
		void set(const string &section, const string &item, const string &value);

		void markUsed(const string &section, const string &item);
		void markUnused(const string &section, const string &item);
		bool isUsed(const string &section, const string &item) const;
		bool anyUnused(string &section, string &item) const;
		bool anyUnused(string &section, string &item, string &value) const;
		void checkUnused() const;

		void clear();
		void merge(const Ini *source); // items in this are overridden by items in source
		void parseCmd(uint32 argc, const char *const args[]); // clears this before parsing
		void load(const string &filename); // clears this before loading
		void save(const string &filename) const;

#define GCHL_GENERATE(TYPE, NAME, DEF) \
		void set##NAME (const string &section, const string &item, const TYPE &value); \
		TYPE get##NAME (const string &section, const string &item, const TYPE &defaul = DEF) const; \
		TYPE cmd##NAME (char shortName, const string &longName, const TYPE &defaul) const; \
		TYPE cmd##NAME (char shortName, const string &longName) const;
		GCHL_GENERATE(bool, Bool, false);
		GCHL_GENERATE(sint32, Sint32, 0);
		GCHL_GENERATE(uint32, Uint32, 0);
		GCHL_GENERATE(sint64, Sint64, 0);
		GCHL_GENERATE(uint64, Uint64, 0);
		GCHL_GENERATE(float, Float, 0);
		GCHL_GENERATE(double, Double, 0);
		GCHL_GENERATE(string, String, "");
#undef GCHL_GENERATE

		Holder<PointerRange<string>> cmdArray(char shortName, const string &longName) const;
	};

	CAGE_CORE_API Holder<Ini> newIni();
	CAGE_CORE_API Holder<Ini> newIni(MemoryArena arena);
	CAGE_CORE_API Holder<Ini> newIni(const string &filename);
	CAGE_CORE_API Holder<Ini> newIni(MemoryArena arena, const string &filename);
	CAGE_CORE_API Holder<Ini> newIni(uint32 argc, const char *const args[]);
	CAGE_CORE_API Holder<Ini> newIni(MemoryArena arena, uint32 argc, const char *const args[]);
}

#endif // guard_ini_h_c866b123_b27e_4758_ab8e_702ef8f315de_
