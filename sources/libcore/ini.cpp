#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/macros.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>

#include <map>
#include <vector>

namespace cage
{
	namespace
	{
		template<class Value>
		using ContainerMap = std::map<String, Value, StringComparatorFast>;

		struct IniValue
		{
			String value;
			bool used = false;
			IniValue() {}
			IniValue(const String &value) : value(value) {}
		};

		struct IniSection
		{
			ContainerMap<IniValue> items;
		};

		class IniImpl : public Ini
		{
		public:
			ContainerMap<IniSection> sections;
			std::vector<String> helps;
		};
	}

	uint32 Ini::sectionsCount() const
	{
		IniImpl *impl = (IniImpl *)this;
		return numeric_cast<uint32>(impl->sections.size());
	}

	String Ini::section(uint32 section) const
	{
		IniImpl *impl = (IniImpl *)this;
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

	bool Ini::sectionExists(const String &section) const
	{
		const IniImpl *impl = (const IniImpl *)this;
		return impl->sections.count(section);
	}

	Holder<PointerRange<String>> Ini::sections() const
	{
		const IniImpl *impl = (const IniImpl *)this;
		PointerRangeHolder<String> tmp;
		tmp.reserve(impl->sections.size());
		for (auto &it : impl->sections)
			tmp.push_back(it.first);
		return tmp;
	}

	void Ini::sectionRemove(const String &section)
	{
		IniImpl *impl = (IniImpl *)this;
		impl->sections.erase(section);
	}

	uint32 Ini::itemsCount(const String &section) const
	{
		if (!sectionExists(section))
			return 0;
		const IniImpl *impl = (const IniImpl *)this;
		return numeric_cast<uint32>(impl->sections.at(section).items.size());
	}

	String Ini::item(const String &section, uint32 item) const
	{
		if (!sectionExists(section))
			return "";
		const IniImpl *impl = (const IniImpl *)this;
		auto i = impl->sections.at(section).items.cbegin();
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

	bool Ini::itemExists(const String &section, const String &item) const
	{
		if (!sectionExists(section))
			return false;
		const IniImpl *impl = (const IniImpl *)this;
		return impl->sections.at(section).items.count(item);
	}

	Holder<PointerRange<String>> Ini::items(const String &section) const
	{
		const IniImpl *impl = (const IniImpl *)this;
		PointerRangeHolder<String> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections.at(section).items;
		tmp.reserve(cont.size());
		for (const auto &it : cont)
			tmp.push_back(it.first);
		return tmp;
	}

	Holder<PointerRange<String>> Ini::values(const String &section) const
	{
		const IniImpl *impl = (const IniImpl *)this;
		PointerRangeHolder<String> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections.at(section).items;
		tmp.reserve(cont.size());
		for (const auto &it : cont)
			tmp.push_back(it.second.value);
		return tmp;
	}

	void Ini::itemRemove(const String &section, const String &item)
	{
		IniImpl *impl = (IniImpl *)this;
		if (sectionExists(section))
			impl->sections[section].items.erase(item);
	}

	String Ini::get(const String &section, const String &item) const
	{
		if (!itemExists(section, item))
			return "";
		const IniImpl *impl = (const IniImpl *)this;
		return impl->sections.at(section).items.at(item).value;
	}

	namespace
	{
		void validateString(const String &str)
		{
			if (str.empty() || find(str, '#') != m || find(str, '[') != m || find(str, ']') != m || find(str, '=') != m)
				CAGE_THROW_ERROR(Exception, "invalid name");
		}
	}

	void Ini::set(const String &section, const String &item, const String &value)
	{
		validateString(section);
		validateString(item);
		if (find(value, '#') != m)
			CAGE_THROW_ERROR(Exception, "invalid value");
		IniImpl *impl = (IniImpl *)this;
		impl->sections[section].items[item] = value;
	}

	void Ini::markUsed(const String &section, const String &item)
	{
		CAGE_ASSERT(itemExists(section, item));
		IniImpl *impl = (IniImpl *)this;
		impl->sections[section].items[item].used = true;
	}

	void Ini::markUnused(const String &section, const String &item)
	{
		CAGE_ASSERT(itemExists(section, item));
		IniImpl *impl = (IniImpl *)this;
		impl->sections[section].items[item].used = false;
	}

	bool Ini::isUsed(const String &section, const String &item) const
	{
		CAGE_ASSERT(itemExists(section, item));
		const IniImpl *impl = (const IniImpl *)this;
		return impl->sections.at(section).items.at(item).used;
	}

	bool Ini::anyUnused(String &section, String &item) const
	{
		String value;
		return anyUnused(section, item, value);
	}

	bool Ini::anyUnused(String &section, String &item, String &value) const
	{
		const IniImpl *impl = (const IniImpl *)this;
		for (const auto &s : impl->sections)
		{
			for (const auto &t : s.second.items)
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
		String section, item, value;
		if (anyUnused(section, item, value))
		{
			CAGE_LOG_THROW(Stringizer() + "section: '" + section + "', item: '" + item + "', " + "value: '" + value + "'");
			CAGE_THROW_ERROR(Exception, "unused ini/config item");
		}
	}

	void Ini::logHelp() const
	{
		const IniImpl *impl = (const IniImpl *)this;
		CAGE_LOG(SeverityEnum::Info, "help", "command line options:");
		for (const String &h : impl->helps)
			CAGE_LOG_CONTINUE(SeverityEnum::Info, "help", h);
	}

	void Ini::checkUnusedWithHelp() const
	{
		try
		{
			checkUnused();
		}
		catch (const Exception &)
		{
			logHelp();
			throw;
		}
	}

	void Ini::clear()
	{
		IniImpl *impl = (IniImpl *)this;
		impl->sections.clear();
		impl->helps.clear();
	}

	void Ini::merge(const Ini *source)
	{
		for (const String &s : source->sections())
		{
			for (const String &i : source->items(s))
				set(s, i, source->get(s, i));
		}
	}

	namespace
	{
		void checkCmdOption(Ini *ini, String &prev, const String &current)
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
			String option = "--";
			for (uint32 i = 1; i < argc; i++)
			{
				String s = args[i];
				CAGE_ASSERT(!s.empty());
				if (!argumentsOnly)
				{
					if (isPattern(s, "---", "", ""))
						CAGE_THROW_ERROR(Exception, "invalid option prefix (---)");
					if (s == "-")
						CAGE_THROW_ERROR(Exception, "missing option name");
					if (s == "--")
					{
						argumentsOnly = true;
						checkCmdOption(this, option, "--");
						continue;
					}
					if (isPattern(s, "--", "", ""))
					{
						String o = remove(s, 0, 2);
						checkCmdOption(this, option, o);
						continue;
					}
					if (isPattern(s, "-", "", ""))
					{
						for (uint32 i = 1, e = s.length(); i != e; i++)
						{
							String o = String({ &s[i], &s[i] + 1 });
							checkCmdOption(this, option, o);
						}
						continue;
					}
				}
				set(option, Stringizer() + itemsCount(option), s);
			}
			checkCmdOption(this, option, "--");
		}
		catch (...)
		{
			CAGE_LOG_THROW(Stringizer() + "failed to parse command line arguments:");
			for (uint32 i = 0; i < argc; i++)
				CAGE_LOG_CONTINUE(SeverityEnum::Note, "exception", args[i]);
			throw;
		}
	}

	void Ini::importBuffer(PointerRange<const char> buffer)
	{
		clear();
		Deserializer des(buffer);
		String sec = "";
		uint32 secIndex = 0;
		uint32 itemIndex = 0;
		for (String line; des.readLine(line);)
		{
			if (line.empty())
				continue;
			uint32 pos = find(line, '#');
			if (pos != m)
				line = subString(line, 0, pos);
			line = trim(line);
			if (line.empty())
				continue;
			if (line[0] == '[' && line[line.length() - 1] == ']')
			{
				itemIndex = 0;
				sec = trim(subString(line, 1, line.length() - 2));
				if (sec.empty())
					sec = Stringizer() + secIndex++;
				if (sectionExists(sec))
				{
					CAGE_LOG_THROW(Stringizer() + "section: '" + sec + "'");
					CAGE_THROW_ERROR(Exception, "duplicate section");
				}
				continue;
			}
			if (sec.empty())
				CAGE_THROW_ERROR(Exception, "item outside section");
			pos = find(line, '=');
			String itemName, itemValue;
			if (pos == m)
				itemValue = line;
			else
			{
				itemName = trim(subString(line, 0, pos));
				itemValue = trim(subString(line, pos + 1, m));
			}
			if (itemName.empty())
				itemName = Stringizer() + itemIndex++;
			if (itemExists(sec, itemName))
			{
				CAGE_LOG_THROW(Stringizer() + "section: '" + sec + "', item: '" + itemName + "'");
				CAGE_THROW_ERROR(Exception, "duplicate item name");
			}
			set(sec, itemName, itemValue);
		}
	}

	void Ini::importFile(const String &filename)
	{
		Holder<File> file = readFile(filename);
		try
		{
			Holder<PointerRange<char>> buff = file->readAll();
			importBuffer(buff);
		}
		catch (...)
		{
			CAGE_LOG_THROW(Stringizer() + "failed to load ini file: '" + filename + "'");
			throw;
		}
	}

	Holder<PointerRange<char>> Ini::exportBuffer() const
	{
		const IniImpl *impl = (const IniImpl *)this;

		std::map<String, std::map<String, String, StringComparatorNatural>, StringComparatorNatural> copy;
		for (const auto &i : impl->sections)
		{
			auto &s = copy[i.first];
			for (const auto &j : i.second.items)
				s[j.first] = j.second.value;
		}

		MemoryBuffer buff(0, 100000);
		Serializer ser(buff);
		for (const auto &i : copy)
		{
			ser.writeLine(String() + "[" + i.first + "]");
			for (const auto &j : i.second)
				ser.writeLine(String() + j.first + " = " + j.second);
			ser.writeLine("");
		}
		return std::move(buff);
	}

	void Ini::exportFile(const String &filename) const
	{
		Holder<PointerRange<char>> buff = exportBuffer();
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> file = newFile(filename, fm);
		file->write(buff);
		file->close();
	}

	namespace
	{
		String toShortName(char c)
		{
			if (c == 0)
				return "";
			return String({ &c, &c + 1 });
		}

		String formatOptionNames(const String &shortName, const String &longName)
		{
			return shortName.empty() ? Stringizer() + "--" + longName : Stringizer() + "--" + longName + " (-" + shortName + ")";
		}

		void addHelp(Ini *ini, const String &shortName, const String &longName, const char *typeName)
		{
			const String h = Stringizer() + formatOptionNames(shortName, longName) + ": " + typeName;
			((IniImpl *)ini)->helps.push_back(h);
		}

		String getCmd(Ini *ini, const String &shortName, const String &longName, const char *typeName)
		{
			addHelp(ini, shortName, longName, typeName);
			const uint32 cnt = ini->itemsCount(shortName) + ini->itemsCount(longName);
			if (cnt > 1)
			{
				CAGE_LOG_THROW(Stringizer() + "option name: " + formatOptionNames(shortName, longName));
				CAGE_THROW_ERROR(Exception, "cmd option contains multiple values");
			}
			if (cnt == 0)
				return "";
			const String a = ini->get(shortName, "0");
			const String b = ini->get(longName, "0");
			const bool ae = a.empty();
			const bool be = b.empty();
			if (ae && be)
			{
				CAGE_LOG_THROW(Stringizer() + "option name: " + formatOptionNames(shortName, longName));
				CAGE_THROW_ERROR(Exception, "invalid item names for cmd options");
			}
			CAGE_ASSERT(ae != be);
			if (!ae)
				ini->markUsed(shortName, "0");
			if (!be)
				ini->markUsed(longName, "0");
			if (ae)
				return b;
			return a;
		}
	}

#define GCHL_GENERATE(TYPE, NAME, TO) \
	void Ini::CAGE_JOIN(set, NAME)(const String &section, const String &item, const TYPE &value) \
	{ \
		set(section, item, Stringizer() + value); \
	}; \
	TYPE Ini::CAGE_JOIN(get, NAME)(const String &section, const String &item, const TYPE &defaul) const \
	{ \
		const String tmp = get(section, item); \
		if (tmp.empty()) \
			return defaul; \
		const_cast<Ini *>(this)->markUsed(section, item); \
		return TO(tmp); \
	} \
	TYPE Ini::CAGE_JOIN(cmd, NAME)(char shortName, const String &longName, const TYPE &defaul) \
	{ \
		const String sn = toShortName(shortName); \
		const String tmp = getCmd(this, sn, longName, CAGE_STRINGIZE(TYPE)); \
		if (tmp.empty()) \
			return defaul; \
		return TO(tmp); \
	} \
	TYPE Ini::CAGE_JOIN(cmd, NAME)(char shortName, const String &longName) \
	{ \
		const String sn = toShortName(shortName); \
		const String tmp = getCmd(this, sn, longName, CAGE_STRINGIZE(TYPE)); \
		if (tmp.empty()) \
		{ \
			CAGE_LOG_THROW(Stringizer() + "option name: " + formatOptionNames(sn, longName)); \
			CAGE_THROW_ERROR(Exception, "missing required cmd option"); \
		} \
		return TO(tmp); \
	}
	GCHL_GENERATE(bool, Bool, toBool);
	GCHL_GENERATE(sint32, Sint32, toSint32);
	GCHL_GENERATE(uint32, Uint32, toUint32);
	GCHL_GENERATE(sint64, Sint64, toSint64);
	GCHL_GENERATE(uint64, Uint64, toUint64);
	GCHL_GENERATE(float, Float, toFloat);
	GCHL_GENERATE(double, Double, toDouble);
	GCHL_GENERATE(String, String, );
#undef GCHL_GENERATE

	Holder<PointerRange<String>> Ini::cmdArray(char shortName, const String &longName)
	{
		const String sn = toShortName(shortName);
		const auto s = values(sn);
		const auto l = values(longName);
		for (const String &item : items(sn))
			markUsed(sn, item);
		for (const String &item : items(longName))
			markUsed(longName, item);
		PointerRangeHolder<String> tmp;
		tmp.reserve(s.size() + l.size());
		tmp.insert(tmp.end(), s.begin(), s.end());
		tmp.insert(tmp.end(), l.begin(), l.end());
		return tmp;
	}

	Holder<Ini> newIni()
	{
		return systemMemory().createImpl<Ini, IniImpl>();
	}
}
