#include <map>
#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/files.h>
#include <cage-core/configIni.h>
#include <cage-core/pointerRangeHolder.h>

namespace cage
{
	namespace
	{
		template<class Value>
		struct containerMap : public std::map<string, Value, stringComparatorFast, memoryArenaStd<std::pair<const string, Value>>>
		{
			typedef typename containerMap::map base;
			containerMap(memoryArena arena) : base(stringComparatorFast(), arena) {}
		};

		struct iniValue
		{
			string value;
			bool used;
			iniValue() : used(false) {}
			iniValue(const string &value) : value(value), used(false) {}
		};

		struct iniSection
		{
			iniSection(memoryArena arena) : items(arena) {}
			containerMap<iniValue> items;
		};

		class iniImpl : public configIni
		{
		public:
			iniImpl(memoryArena arena) : arena(arena), sections(arena) {}
			memoryArena arena;
			containerMap<holder<iniSection>> sections;
		};
	}

	uint32 configIni::sectionsCount() const
	{
		iniImpl *impl = (iniImpl*)this;
		return numeric_cast<uint32>(impl->sections.size());
	}

	string configIni::section(uint32 section) const
	{
		iniImpl *impl = (iniImpl*)this;
		auto i = impl->sections.cbegin();
		try
		{
			std::advance(i, section);
		}
		catch (...)
		{
			CAGE_THROW_ERROR(exception, "invalid ini section index");
		}
		return i->first;
	}

	bool configIni::sectionExists(const string &section) const
	{
		iniImpl *impl = (iniImpl*)this;
		return impl->sections.count(section);
	}

	holder<pointerRange<string>> configIni::sections() const
	{
		iniImpl *impl = (iniImpl*)this;
		pointerRangeHolder<string> tmp;
		tmp.reserve(impl->sections.size());
		for (auto &it : impl->sections)
			tmp.push_back(it.first);
		return tmp;
	}

	void configIni::sectionRemove(const string &section)
	{
		iniImpl *impl = (iniImpl*)this;
		impl->sections.erase(section);
	}

	uint32 configIni::itemsCount(const string &section) const
	{
		if (!sectionExists(section))
			return 0;
		iniImpl *impl = (iniImpl*)this;
		return numeric_cast<uint32>(impl->sections[section]->items.size());
	}

	string configIni::item(const string &section, uint32 item) const
	{
		if (!sectionExists(section))
			return "";
		iniImpl *impl = (iniImpl*)this;
		auto i = impl->sections[section]->items.cbegin();
		try
		{
			std::advance(i, item);
		}
		catch (...)
		{
			CAGE_THROW_ERROR(exception, "invalid ini item index");
		}
		return i->first;
	}

	bool configIni::itemExists(const string &section, const string &item) const
	{
		if (!sectionExists(section))
			return false;
		iniImpl *impl = (iniImpl*)this;
		return impl->sections[section]->items.count(item);
	}

	holder<pointerRange<string>> configIni::items(const string &section) const
	{
		iniImpl *impl = (iniImpl*)this;
		pointerRangeHolder<string> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections[section]->items;
		tmp.reserve(cont.size());
		for (auto it : cont)
			tmp.push_back(it.first);
		return tmp;
	}

	holder<pointerRange<string>> configIni::values(const string &section) const
	{
		iniImpl *impl = (iniImpl*)this;
		pointerRangeHolder<string> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections[section]->items;
		tmp.reserve(cont.size());
		for (const auto &it : cont)
			tmp.push_back(it.second.value);
		return tmp;
	}

	void configIni::itemRemove(const string &section, const string &item)
	{
		iniImpl *impl = (iniImpl*)this;
		if (sectionExists(section))
			impl->sections[section]->items.erase(item);
	}

	string configIni::get(const string &section, const string &item) const
	{
		if (!itemExists(section, item))
			return "";
		iniImpl *impl = (iniImpl*)this;
		return impl->sections[section]->items[item].value;
	}

	namespace
	{
		void validateString(const string &str)
		{
			if (str.empty() || str.find('#') != m || str.find('[') != m || str.find(']') != m || str.find('=') != m)
				CAGE_THROW_ERROR(exception, "invalid name");
		}
	}

	void configIni::set(const string &section, const string &item, const string &value)
	{
		validateString(section);
		validateString(item);
		if (value.find('#') != m)
			CAGE_THROW_ERROR(exception, "invalid value");
		iniImpl *impl = (iniImpl*)this;
		if (!sectionExists(section))
			impl->sections[section] = impl->arena.createHolder<iniSection>(impl->arena);
		impl->sections[section]->items[item] = value;
	}

	void configIni::markUsed(const string &section, const string &item)
	{
		CAGE_ASSERT(itemExists(section, item), section, item);
		iniImpl *impl = (iniImpl*)this;
		impl->sections[section]->items[item].used = true;
	}

	void configIni::markUnused(const string &section, const string &item)
	{
		CAGE_ASSERT(itemExists(section, item), section, item);
		iniImpl *impl = (iniImpl*)this;
		impl->sections[section]->items[item].used = false;
	}

	bool configIni::isUsed(const string &section, const string &item) const
	{
		CAGE_ASSERT(itemExists(section, item), section, item);
		iniImpl *impl = (iniImpl*)this;
		return impl->sections[section]->items[item].used;
	}

	bool configIni::anyUnused(string &section, string &item) const
	{
		string value;
		return anyUnused(section, item, value);
	}

	bool configIni::anyUnused(string &section, string &item, string &value) const
	{
		iniImpl *impl = (iniImpl*)this;
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

	void configIni::checkUnused() const
	{
		string section, item, value;
		if (anyUnused(section, item, value))
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "section: '" + section + "', item: '" + item + "', " + "value: '" + value + "'");
			CAGE_THROW_ERROR(exception, "unused ini/config item");
		}
	}

	void configIni::clear()
	{
		iniImpl *impl = (iniImpl*)this;
		impl->sections.clear();
	}

	void configIni::merge(const configIni *source)
	{
		for (string s : source->sections())
		{
			for (string i : source->items(s))
				set(s, i, source->get(s, i));
		}
	}

	namespace
	{
		void checkCmdOption(configIni *ini, string &prev, const string &current)
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

	void configIni::parseCmd(uint32 argc, const char *const args[])
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
						CAGE_THROW_ERROR(exception, "invalid option prefix (---)");
					if (s == "-")
						CAGE_THROW_ERROR(exception, "missing option name");
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
			CAGE_LOG(severityEnum::Note, "exception", stringizer() + "failed to parse command line arguments:");
			for (uint32 i = 0; i < argc; i++)
				CAGE_LOG_CONTINUE(severityEnum::Note, "exception", args[i]);
			throw;
		}
	}

	void configIni::load(const string &filename)
	{
		holder<fileHandle> file = newFile(filename, fileMode(true, false));
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
						CAGE_THROW_ERROR(exception, "duplicate section");
					continue;
				}
				if (sec.empty())
					CAGE_THROW_ERROR(exception, "item outside section");
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
					CAGE_THROW_ERROR(exception, "duplicate item name");
				set(sec, itemName, itemValue);
			}
		}
		catch (...)
		{
			CAGE_LOG(severityEnum::Note, "exception", stringizer() + "failed to load ini file: '" + filename + "'");
			throw;
		}
	}

	void configIni::save(const string &filename) const
	{
		iniImpl *impl = (iniImpl*)this;
		fileMode fm(false, true);
		fm.textual = true;
		holder<fileHandle> file = newFile(filename, fm);
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

		string getCmd(const configIni *ini, string shortName, const string &longName)
		{
			uint32 cnt = ini->itemsCount(shortName) + ini->itemsCount(longName);
			if (cnt > 1)
				CAGE_THROW_ERROR(exception, "cmd option contains multiple values");
			if (cnt == 0)
				return "";
			string a = ini->get(shortName, "0");
			string b = ini->get(longName, "0");
			bool ae = a.empty();
			bool be = b.empty();
			if (ae && be)
				CAGE_THROW_ERROR(exception, "invalid item names for cmd options");
			CAGE_ASSERT(ae != be);
			if (!ae) const_cast<configIni*>(ini)->markUsed(shortName, "0");
			if (!be) const_cast<configIni*>(ini)->markUsed(longName, "0");
			if (ae)
				return b;
			return a;
		}
	}

#define GCHL_GENERATE(TYPE, NAME, TO) \
	void configIni::CAGE_JOIN(set, NAME) (const string &section, const string &item, const TYPE &value) \
	{ \
		set(section, item, string(value)); \
	}; \
	TYPE configIni::CAGE_JOIN(get, NAME) (const string &section, const string &item, const TYPE &defaul) const \
	{ \
		string tmp = get(section, item); \
		if (tmp.empty()) \
			return defaul; \
		const_cast<configIni*>(this)->markUsed(section, item); \
		return tmp TO; \
	} \
	TYPE configIni::CAGE_JOIN(cmd, NAME) (char shortName, const string &longName, const TYPE &defaul) const \
	{ \
		string sn = toShortName(shortName); \
		try \
		{ \
			string tmp = getCmd(this, sn, longName); \
			if (tmp.empty()) \
				return defaul; \
			return tmp TO; \
		} \
		catch (const exception &) \
		{ \
			CAGE_LOG(severityEnum::Note, "exception", string() + "cmd option: '" + longName + "' (" + sn + ")"); \
			throw; \
		} \
	} \
	TYPE configIni::CAGE_JOIN(cmd, NAME) (char shortName, const string &longName) const \
	{ \
		string sn = toShortName(shortName); \
		try \
		{ \
			string tmp = getCmd(this, sn, longName); \
			if (tmp.empty()) \
				CAGE_THROW_ERROR(exception, "missing required cmd option"); \
			return tmp TO; \
		} \
		catch (const exception &) \
		{ \
			CAGE_LOG(severityEnum::Note, "exception", string() + "cmd option: '" + longName + "' (" + sn + ")"); \
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

	holder<pointerRange<string>> configIni::cmdArray(char shortName, const string &longName) const
	{
		const string sn = toShortName(shortName);
		const auto s = values(sn);
		const auto l = values(longName);
		for (const string &item : items(sn))
			const_cast<configIni*>(this)->markUsed(sn, item);
		for (const string &item : items(longName))
			const_cast<configIni*>(this)->markUsed(longName, item);
		pointerRangeHolder<string> tmp;
		tmp.reserve(s.size() + l.size());
		tmp.insert(tmp.end(), s.begin(), s.end());
		tmp.insert(tmp.end(), l.begin(), l.end());
		return tmp;
	}

	holder<configIni> newConfigIni()
	{
		return newConfigIni(detail::systemArena());
	}

	holder<configIni> newConfigIni(memoryArena arena)
	{
		return detail::systemArena().createImpl<configIni, iniImpl>(arena);
	}

	holder<configIni> newConfigIni(const string &filename)
	{
		return newConfigIni(detail::systemArena(), filename);
	}

	holder<configIni> newConfigIni(memoryArena arena, const string &filename)
	{
		holder<configIni> ini = newConfigIni(arena);
		ini->load(filename);
		return ini;
	}

	holder<configIni> newConfigIni(uint32 argc, const char *const args[])
	{
		return newConfigIni(detail::systemArena(), argc, args);
	}

	holder<configIni> newConfigIni(memoryArena arena, uint32 argc, const char *const args[])
	{
		holder<configIni> ini = newConfigIni(arena);
		ini->parseCmd(argc, args);
		return ini;
	}
}
