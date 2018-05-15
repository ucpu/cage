#ifndef guard_iniReader_h_c866b123_b27e_4758_ab8e_702ef8f315de_
#define guard_iniReader_h_c866b123_b27e_4758_ab8e_702ef8f315de_

/*
	char '#' denotes a comment (to end of line)
	empty lines (or lines containing only white chars ' ' and '\t') are skipped
	chars '\\', '\'' and '"' have no special meaning
	section names, item names and values are trimmed from both sides
	section names and item names may not contain any of '#', '=', '[' or ']'
	values may not contain '#'
	duplicate sections or duplicate items inside same section are invalid
	items before first section are invalid
	empty sections "[]" are converted to numerical sequences (beware of duplicities) - only works when loading from file
	empty items "value" or "=value" are converted to numerical sequences (beware of duplicities and values containing '=') - only works when loading from file
*/

namespace cage
{
	class CAGE_API iniClass
	{
	public:
		uint32 sectionCount() const;
		string section(uint32 section) const;
		bool sectionExists(const string &section) const;
		pointerRange<string> sections() const;
		void sectionRemove(const string &section);
		uint32 itemCount(const string &section) const;
		string item(const string &section, uint32 item) const;
		bool itemExists(const string &section, const string &item) const;
		pointerRange<string> items(const string &section) const;
		void itemRemove(const string &section, const string &item);

		string get(const string &section, const string &item) const;
		void set(const string &section, const string &item, const string &value);

		void clear();
		void load(const string &filename);
		void merge(const iniClass *source);
		void save(const string &filename) const;

#define GCHL_GENERATE(TYPE, NAME, DEF, TO) \
		TYPE CAGE_JOIN(get, NAME) (const string &section, const string &item, const TYPE &defaul = DEF) const \
		{ \
			string tmp = get(section, item); \
			if (tmp.empty()) \
				return defaul; \
			return tmp TO; \
		} \
		void CAGE_JOIN(set, NAME) (const string &section, const string &item, const TYPE &value) \
		{ \
			set(section, item, string(value)); \
		};
		GCHL_GENERATE(bool, Bool, false, .toBool());
		GCHL_GENERATE(sint32, Sint32, 0, .toSint32());
		GCHL_GENERATE(uint32, Uint32, 0, .toUint32());
		GCHL_GENERATE(sint64, Sint64, 0, .toSint64());
		GCHL_GENERATE(uint64, Uint64, 0, .toUint64());
		GCHL_GENERATE(float, Float, 0, .toFloat());
		GCHL_GENERATE(double, Double, 0, .toDouble());
		GCHL_GENERATE(string, String, "", );
#undef GCHL_GENERATE
	};

	CAGE_API holder<iniClass> newIni(uintPtr memory);
	CAGE_API holder<iniClass> newIni(memoryArena arena);
	CAGE_API holder<iniClass> newIni(); // uses system memory arena
}

#endif // guard_iniReader_h_c866b123_b27e_4758_ab8e_702ef8f315de_
