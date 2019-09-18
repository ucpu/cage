#include <map>
#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
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
				set(option, itemsCount(option), s);
			}
			checkCmdOption(this, option, "--");
		}
		catch (...)
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "failed to parse command line arguments:");
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
			CAGE_LOG(severityEnum::Note, "exception", string() + "failed to load ini file: '" + filename + "'");
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
