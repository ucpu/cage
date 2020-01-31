#include <cage-core/memory.h>
#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/macros.h>

#include <map>
#include <vector>

namespace cage
{
	namespace
	{
		template<class Value>
		struct ContainerMap : public std::map<string, Value, stringComparatorFast, MemoryArenaStd<std::pair<const string, Value>>>
		{
			typedef typename ContainerMap::map Base;
			ContainerMap(MemoryArena arena) : Base(stringComparatorFast(), arena) {}
		};

		struct IniValue
		{
			string value;
			bool used;
			IniValue() : used(false) {}
			IniValue(const string &value) : value(value), used(false) {}
		};

		struct IniSection
		{
			IniSection(MemoryArena arena) : items(arena) {}
			ContainerMap<IniValue> items;
		};

		class IniImpl : public Ini
		{
		public:
			IniImpl(MemoryArena arena) : arena(arena), sections(arena) {}
			MemoryArena arena;
			ContainerMap<Holder<IniSection>> sections;
		};
	}

	uint32 Ini::sectionsCount() const
	{
		IniImpl *impl = (IniImpl*)this;
		return numeric_cast<uint32>(impl->sections.size());
	}

	string Ini::section(uint32 section) const
	{
		IniImpl *impl = (IniImpl*)this;
		auto i = impl->sections.cbegin();
		try
		{
			std::advance(i, section);
		}
		catch (...)
		{
			CAGE_THROW_ERROR(Exception, "invalid ini section index");
		}
		return i->first;
	}

	bool Ini::sectionExists(const string &section) const
	{
		IniImpl *impl = (IniImpl*)this;
		return impl->sections.count(section);
	}

	Holder<PointerRange<string>> Ini::sections() const
	{
		IniImpl *impl = (IniImpl*)this;
		PointerRangeHolder<string> tmp;
		tmp.reserve(impl->sections.size());
		for (auto &it : impl->sections)
			tmp.push_back(it.first);
		return tmp;
	}

	void Ini::sectionRemove(const string &section)
	{
		IniImpl *impl = (IniImpl*)this;
		impl->sections.erase(section);
	}

	uint32 Ini::itemsCount(const string &section) const
	{
		if (!sectionExists(section))
			return 0;
		IniImpl *impl = (IniImpl*)this;
		return numeric_cast<uint32>(impl->sections[section]->items.size());
	}

	string Ini::item(const string &section, uint32 item) const
	{
		if (!sectionExists(section))
			return "";
		IniImpl *impl = (IniImpl*)this;
		auto i = impl->sections[section]->items.cbegin();
		try
		{
			std::advance(i, item);
		}
		catch (...)
		{
			CAGE_THROW_ERROR(Exception, "invalid ini item index");
		}
		return i->first;
	}

	bool Ini::itemExists(const string &section, const string &item) const
	{
		if (!sectionExists(section))
			return false;
		IniImpl *impl = (IniImpl*)this;
		return impl->sections[section]->items.count(item);
	}

	Holder<PointerRange<string>> Ini::items(const string &section) const
	{
		IniImpl *impl = (IniImpl*)this;
		PointerRangeHolder<string> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections[section]->items;
		tmp.reserve(cont.size());
		for (auto it : cont)
			tmp.push_back(it.first);
		return tmp;
	}

	Holder<PointerRange<string>> Ini::values(const string &section) const
	{
		IniImpl *impl = (IniImpl*)this;
		PointerRangeHolder<string> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections[section]->items;
		tmp.reserve(cont.size());
		for (const auto &it : cont)
			tmp.push_back(it.second.value);
		return tmp;
	}

	void Ini::itemRemove(const string &section, const string &item)
	{
		IniImpl *impl = (IniImpl*)this;
		if (sectionExists(section))
			impl->sections[section]->items.erase(item);
	}

	string Ini::get(const string &section, const string &item) const
	{
		if (!itemExists(section, item))
			return "";
		IniImpl *impl = (IniImpl*)this;
		return impl->sections[section]->items[item].value;
	}

	namespace
	{
		void validateString(const string &str)
		{
			if (str.empty() || str.find('#') != m || str.find('[') != m || str.find(']') != m || str.find('=') != m)
				CAGE_THROW_ERROR(Exception, "invalid name");
		}
	}

	void Ini::set(const string &section, const string &item, const string &value)
	{
		validateString(section);
		validateString(item);
		if (value.find('#') != m)
			CAGE_THROW_ERROR(Exception, "invalid value");
		IniImpl *impl = (IniImpl*)this;
		if (!sectionExists(section))
			impl->sections[section] = impl->arena.createHolder<IniSection>(impl->arena);
		impl->sections[section]->items[item] = value;
	}

	void Ini::markUsed(const string &section, const string &item)
	{
		CAGE_ASSERT(itemExists(section, item), section, item);
		IniImpl *impl = (IniImpl*)this;
		impl->sections[section]->items[item].used = true;
	}

	void Ini::markUnused(const string &section, const string &item)
	{
		CAGE_ASSERT(itemExists(section, item), section, item);
		IniImpl *impl = (IniImpl*)this;
		impl->sections[section]->items[item].used = false;
	}

	bool Ini::isUsed(const string &section, const string &item) const
	{
		CAGE_ASSERT(itemExists(section, item), section, item);
		IniImpl *impl = (IniImpl*)this;
		return impl->sections[section]->items[item].used;
	}

	bool Ini::anyUnused(string &section, string &item) const
	{
		string value;
		return anyUnused(section, item, value);
	}

	bool Ini::anyUnused(string &section, string &item, string &value) const
	{
		IniImpl *impl = (IniImpl*)this;
		for (const auto &s : impl->sections)
		{
			for (const auto &t : s.second->items)
			{
				if (!t.second.used)
				{
					section = s.first;
					item = t.first;
					value = t.second.value;
					return true;
				}
			}
		}
		return false;
	}

	void Ini::checkUnused() const
	{
		string section, item, value;
		if (anyUnused(section, item, value))
		{
			CAGE_LOG(SeverityEnum::Note, "exception", string() + "section: '" + section + "', item: '" + item + "', " + "value: '" + value + "'");
			CAGE_THROW_ERROR(Exception, "unused ini/config item");
		}
	}

	void Ini::clear()
	{
		IniImpl *impl = (IniImpl*)this;
		impl->sections.clear();
	}

	void Ini::merge(const Ini *source)
	{
		for (string s : source->sections())
		{
			for (string i : source->items(s))
				set(s, i, source->get(s, i));
		}
	}

	namespace
	{
		void checkCmdOption(Ini *ini, string &prev, const string &current)
		{
			if (prev != "--")
			{
				if (ini->itemsCount(prev) == 0)
					ini->set(prev, "0", "true");
				if (current != "--")
					ini->sectionRemove(current);
			}
			prev = current;
		}
	}

	void Ini::parseCmd(uint32 argc, const char *const args[])
	{
		clear();
		try
		{
			bool argumentsOnly = false;
			string option = "--";
			for (uint32 i = 1; i < argc; i++)
			{
				string s = args[i];
				CAGE_ASSERT(!s.empty());
				if (!argumentsOnly)
				{
					if (s.isPattern("---", "", ""))
						CAGE_THROW_ERROR(Exception, "invalid option prefix (---)");
					if (s == "-")
						CAGE_THROW_ERROR(Exception, "missing option name");
					if (s == "--")
					{
						argumentsOnly = true;
						checkCmdOption(this, option, "--");
						continue;
					}
					if (s.isPattern("--", "", ""))
					{
						string o = s.remove(0, 2);
						checkCmdOption(this, option, o);
						continue;
					}
					if (s.isPattern("-", "", ""))
					{
						for (uint32 i = 1, e = s.length(); i != e; i++)
						{
							string o = string(&s[i], 1);
							checkCmdOption(this, option, o);
						}
						continue;
					}
				}
				set(option, string(itemsCount(option)), s);
			}
			checkCmdOption(this, option, "--");
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "failed to parse command line arguments:");
			for (uint32 i = 0; i < argc; i++)
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "exception", args[i]);
			throw;
		}
	}

	void Ini::load(const string &filename)
	{
		Holder<File> file = newFile(filename, FileMode(true, false));
		clear();
		try
		{
			string sec = "";
			uint32 secIndex = 0;
			uint32 itemIndex = 0;
			for (string line; file->readLine(line);)
			{
				if (line.empty())
					continue;
				uint32 pos = line.find('#');
				if (pos != m)
					line = line.subString(0, pos);
				line = line.trim();
				if (line.empty())
					continue;
				if (line[0] == '[' && line[line.length() - 1] == ']')
				{
					itemIndex = 0;
					sec = line.subString(1, line.length() - 2).trim();
					if (sec.empty())
						sec = string(secIndex++);
					if (sectionExists(sec))
						CAGE_THROW_ERROR(Exception, "duplicate section");
					continue;
				}
				if (sec.empty())
					CAGE_THROW_ERROR(Exception, "item outside section");
				pos = line.find('=');
				string itemName, itemValue;
				if (pos == m)
					itemValue = line;
				else
				{
					itemName = line.subString(0, pos).trim();
					itemValue = line.subString(pos + 1, m).trim();
				}
				if (itemName.empty())
					itemName = string(itemIndex++);
				if (itemExists(sec, itemName))
					CAGE_THROW_ERROR(Exception, "duplicate item name");
				set(sec, itemName, itemValue);
			}
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "failed to load ini file: '" + filename + "'");
			throw;
		}
	}

	void Ini::save(const string &filename) const
	{
		IniImpl *impl = (IniImpl*)this;
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> file = newFile(filename, fm);
		for (const auto &i : impl->sections)
		{
			file->writeLine(string() + "[" + i.first + "]");
			for (const auto &j : i.second->items)
				file->writeLine(string() + j.first + "=" + j.second.value);
		}
	}

	namespace
	{
		string toShortName(char c)
		{
			if (c == 0)
				return "";
			return string(&c, 1);
		}

		string getCmd(const Ini *ini, string shortName, const string &longName)
		{
			uint32 cnt = ini->itemsCount(shortName) + ini->itemsCount(longName);
			if (cnt > 1)
				CAGE_THROW_ERROR(Exception, "cmd option contains multiple values");
			if (cnt == 0)
				return "";
			string a = ini->get(shortName, "0");
			string b = ini->get(longName, "0");
			bool ae = a.empty();
			bool be = b.empty();
			if (ae && be)
				CAGE_THROW_ERROR(Exception, "invalid item names for cmd options");
			CAGE_ASSERT(ae != be);
			if (!ae) const_cast<Ini*>(ini)->markUsed(shortName, "0");
			if (!be) const_cast<Ini*>(ini)->markUsed(longName, "0");
			if (ae)
				return b;
			return a;
		}
	}

#define GCHL_GENERATE(TYPE, NAME, TO) \
	void Ini::CAGE_JOIN(set, NAME) (const string &section, const string &item, const TYPE &value) \
	{ \
		set(section, item, string(value)); \
	}; \
	TYPE Ini::CAGE_JOIN(get, NAME) (const string &section, const string &item, const TYPE &defaul) const \
	{ \
		string tmp = get(section, item); \
		if (tmp.empty()) \
			return defaul; \
		const_cast<Ini*>(this)->markUsed(section, item); \
		return tmp TO; \
	} \
	TYPE Ini::CAGE_JOIN(cmd, NAME) (char shortName, const string &longName, const TYPE &defaul) const \
	{ \
		string sn = toShortName(shortName); \
		try \
		{ \
			string tmp = getCmd(this, sn, longName); \
			if (tmp.empty()) \
				return defaul; \
			return tmp TO; \
		} \
		catch (const Exception &) \
		{ \
			CAGE_LOG(SeverityEnum::Note, "exception", string() + "cmd option: '" + longName + "' (" + sn + ")"); \
			throw; \
		} \
	} \
	TYPE Ini::CAGE_JOIN(cmd, NAME) (char shortName, const string &longName) const \
	{ \
		string sn = toShortName(shortName); \
		try \
		{ \
			string tmp = getCmd(this, sn, longName); \
			if (tmp.empty()) \
				CAGE_THROW_ERROR(Exception, "missing required cmd option"); \
			return tmp TO; \
		} \
		catch (const Exception &) \
		{ \
			CAGE_LOG(SeverityEnum::Note, "exception", string() + "cmd option: '" + longName + "' (" + sn + ")"); \
			throw; \
		} \
	}
	GCHL_GENERATE(bool, Bool, .toBool());
	GCHL_GENERATE(sint32, Sint32, .toSint32());
	GCHL_GENERATE(uint32, Uint32, .toUint32());
	GCHL_GENERATE(sint64, Sint64, .toSint64());
	GCHL_GENERATE(uint64, Uint64, .toUint64());
	GCHL_GENERATE(float, Float, .toFloat());
	GCHL_GENERATE(double, Double, .toDouble());
	GCHL_GENERATE(string, String, );
#undef GCHL_GENERATE

	Holder<PointerRange<string>> Ini::cmdArray(char shortName, const string &longName) const
	{
		const string sn = toShortName(shortName);
		const auto s = values(sn);
		const auto l = values(longName);
		for (const string &item : items(sn))
			const_cast<Ini*>(this)->markUsed(sn, item);
		for (const string &item : items(longName))
			const_cast<Ini*>(this)->markUsed(longName, item);
		PointerRangeHolder<string> tmp;
		tmp.reserve(s.size() + l.size());
		tmp.insert(tmp.end(), s.begin(), s.end());
		tmp.insert(tmp.end(), l.begin(), l.end());
		return tmp;
	}

	Holder<Ini> newIni()
	{
		return newIni(detail::systemArena());
	}

	Holder<Ini> newIni(MemoryArena arena)
	{
		return detail::systemArena().createImpl<Ini, IniImpl>(arena);
	}

	Holder<Ini> newIni(const string &filename)
	{
		return newIni(detail::systemArena(), filename);
	}

	Holder<Ini> newIni(MemoryArena arena, const string &filename)
	{
		Holder<Ini> ini = newIni(arena);
		ini->load(filename);
		return ini;
	}

	Holder<Ini> newIni(uint32 argc, const char *const args[])
	{
		return newIni(detail::systemArena(), argc, args);
	}

	Holder<Ini> newIni(MemoryArena arena, uint32 argc, const char *const args[])
	{
		Holder<Ini> ini = newIni(arena);
		ini->parseCmd(argc, args);
		return ini;
	}
}
