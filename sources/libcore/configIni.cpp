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
		struct containerMap
		{
			containerMap(memoryArena arena) : cont(stringComparatorFast(), arena) {}
			typedef std::map<string, Value, stringComparatorFast, memoryArenaStd<std::pair<const string, Value>>> contType;
			contType cont;
		};

		struct inisection
		{
			inisection(memoryArena arena) : items(arena) {}
			containerMap<string> items;
		};

		class iniImpl : public configIni
		{
		public:
			iniImpl(memoryArena arena) : arena(arena), sections(arena) {}
			memoryArena arena;
			containerMap<holder<inisection>> sections;
		};
	}

	uint32 configIni::sectionsCount() const
	{
		iniImpl *impl = (iniImpl*)this;
		return numeric_cast<uint32>(impl->sections.cont.size());
	}

	string configIni::section(uint32 section) const
	{
		iniImpl *impl = (iniImpl*)this;
		auto i = impl->sections.cont.cbegin();
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
		return impl->sections.cont.count(section);
	}

	holder<pointerRange<string>> configIni::sections() const
	{
		iniImpl *impl = (iniImpl*)this;
		pointerRangeHolder<string> tmp;
		tmp.reserve(impl->sections.cont.size());
		for (auto &it : impl->sections.cont)
			tmp.push_back(it.first);
		return tmp;
	}

	void configIni::sectionRemove(const string &section)
	{
		iniImpl *impl = (iniImpl*)this;
		impl->sections.cont.erase(section);
	}

	uint32 configIni::itemsCount(const string &section) const
	{
		if (!sectionExists(section))
			return 0;
		iniImpl *impl = (iniImpl*)this;
		return numeric_cast<uint32>(impl->sections.cont[section]->items.cont.size());
	}

	string configIni::item(const string &section, uint32 item) const
	{
		if (!sectionExists(section))
			return "";
		iniImpl *impl = (iniImpl*)this;
		auto i = impl->sections.cont[section]->items.cont.cbegin();
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
		return impl->sections.cont[section]->items.cont.count(item);
	}

	holder<pointerRange<string>> configIni::items(const string &section) const
	{
		iniImpl *impl = (iniImpl*)this;
		pointerRangeHolder<string> tmp;
		if (!sectionExists(section))
			return tmp;
		auto &cont = impl->sections.cont[section]->items.cont;
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
		auto &cont = impl->sections.cont[section]->items.cont;
		tmp.reserve(cont.size());
		for (auto it : cont)
			tmp.push_back(it.second);
		return tmp;
	}

	void configIni::itemRemove(const string &section, const string &item)
	{
		iniImpl *impl = (iniImpl*)this;
		if (sectionExists(section))
			impl->sections.cont[section]->items.cont.erase(item);
	}

	string configIni::get(const string &section, const string &item) const
	{
		if (!itemExists(section, item))
			return "";
		iniImpl *impl = (iniImpl*)this;
		return impl->sections.cont[section]->items.cont[item];
	}

	namespace
	{
		void validateString(const string &str)
		{
			if (str.empty() || str.find('#') != m || str.find('[') != m || str.find(']') != m || str.find('=') != m)
				CAGE_THROW_ERROR(exception, "invalid value");
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
			impl->sections.cont[section] = impl->arena.createHolder<inisection>(impl->arena);
		impl->sections.cont[section]->items.cont[item] = value;
	}

	void configIni::clear()
	{
		iniImpl *impl = (iniImpl*)this;
		impl->sections.cont.clear();
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
				CAGE_ASSERT_RUNTIME(!s.empty());
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
		holder<file> file = newFile(filename, fileMode(true, false));
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
		holder<file> file = newFile(filename, fm);
		for (const auto &i : impl->sections.cont)
		{
			file->writeLine(string() + "[" + i.first + "]");
			for (const auto &j : i.second->items.cont)
				file->writeLine(string() + j.first + "=" + j.second);
		}
	}

	holder<configIni> newConfigIni(memoryArena arena)
	{
		return detail::systemArena().createImpl<configIni, iniImpl>(arena);
	}

	holder<configIni> newConfigIni()
	{
		return detail::systemArena().createImpl<configIni, iniImpl>(detail::systemArena());
	}
}
