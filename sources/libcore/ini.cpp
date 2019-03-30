#include <map>
#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/memory.h>
#include <cage-core/filesystem.h>
#include <cage-core/ini.h>

namespace cage
{
	namespace
	{
		template<class Value> struct containerMap
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

		class iniImpl : public iniClass
		{
		public:
			iniImpl(uintPtr memory) : pool(memory), arena(&pool), sections(arena) {}
			iniImpl(memoryArena arena) : pool(1), arena(arena), sections(arena) {}
			memoryArenaGrowing<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeMap<string, string>)>, memoryConcurrentPolicyNone> pool;
			memoryArena arena;
			containerMap<holder<inisection>> sections;
			std::vector<string> tmpSections, tmpItems;
		};
	}

	uint32 iniClass::sectionCount() const
	{
		iniImpl *impl = (iniImpl*)this;
		return numeric_cast<uint32>(impl->sections.cont.size());
	}

	string iniClass::section(uint32 section) const
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

	bool iniClass::sectionExists(const string &section) const
	{
		iniImpl *impl = (iniImpl*)this;
		return impl->sections.cont.count(section);
	}

	pointerRange<string> iniClass::sections() const
	{
		iniImpl *impl = (iniImpl*)this;
		impl->tmpSections.clear();
		for (auto &it : impl->sections.cont)
			impl->tmpSections.push_back(it.first);
		return impl->tmpSections;
	}

	void iniClass::sectionRemove(const string &section)
	{
		iniImpl *impl = (iniImpl*)this;
		impl->sections.cont.erase(section);
	}

	uint32 iniClass::itemCount(const string &section) const
	{
		if (!sectionExists(section))
			return 0;
		iniImpl *impl = (iniImpl*)this;
		return numeric_cast<uint32>(impl->sections.cont[section]->items.cont.size());
	}

	string iniClass::item(const string &section, uint32 item) const
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

	bool iniClass::itemExists(const string &section, const string &item) const
	{
		if (!sectionExists(section))
			return false;
		iniImpl *impl = (iniImpl*)this;
		return impl->sections.cont[section]->items.cont.count(item);
	}

	pointerRange<string> iniClass::items(const string &section) const
	{
		iniImpl *impl = (iniImpl*)this;
		impl->tmpItems.clear();
		if (sectionExists(section))
			for (auto it : impl->sections.cont[section]->items.cont)
				impl->tmpItems.push_back(it.first);
		return impl->tmpItems;
	}

	void iniClass::itemRemove(const string &section, const string &item)
	{
		iniImpl *impl = (iniImpl*)this;
		if (sectionExists(section))
			impl->sections.cont[section]->items.cont.erase(item);
	}

	string iniClass::get(const string &section, const string &item) const
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
			if (str.empty() || str.find('#') != -1 || str.find('[') != -1 || str.find(']') != -1 || str.find('=') != -1)
				CAGE_THROW_ERROR(exception, "invalid value");
		}
	}

	void iniClass::set(const string &section, const string &item, const string &value)
	{
		validateString(section);
		validateString(item);
		if (value.find('#') != -1)
			CAGE_THROW_ERROR(exception, "invalid value");
		iniImpl *impl = (iniImpl*)this;
		if (!sectionExists(section))
			impl->sections.cont[section] = impl->arena.createHolder<inisection>(impl->arena);
		impl->sections.cont[section]->items.cont[item] = value;
	}

	void iniClass::clear()
	{
		iniImpl *impl = (iniImpl*)this;
		impl->sections.cont.clear();
	}

	void iniClass::load(const string &filename)
	{
		iniImpl *impl = (iniImpl*)this;
		holder<fileClass> file = newFile(filename, fileMode(true, false));
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
				if (pos != -1)
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
				if (pos == -1)
					itemValue = line;
				else
				{
					itemName = line.subString(0, pos).trim();
					itemValue = line.subString(pos + 1, -1).trim();
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

	void iniClass::merge(const iniClass *source)
	{
		for (string s : source->sections())
		{
			for (string i : source->items(s))
				set(s, i, source->get(s, i));
		}
	}

	void iniClass::save(const string &filename) const
	{
		iniImpl *impl = (iniImpl*)this;
		fileMode fm(false, true);
		fm.textual = true;
		holder<fileClass> file = newFile(filename, fm);
		for (const auto &i : impl->sections.cont)
		{
			file->writeLine(string() + "[" + i.first + "]");
			for (const auto &j : i.second->items.cont)
				file->writeLine(string() + j.first + "=" + j.second);
		}
	}

	holder<iniClass> newIni(uintPtr memory)
	{
		return detail::systemArena().createImpl<iniClass, iniImpl>(memory);
	}

	holder<iniClass> newIni(memoryArena arena)
	{
		return detail::systemArena().createImpl<iniClass, iniImpl>(arena);
	}

	holder<iniClass> newIni()
	{
		return detail::systemArena().createImpl<iniClass, iniImpl>(detail::systemArena());
	}
}
