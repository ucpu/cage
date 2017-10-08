#include <map>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/ini.h>

namespace cage
{
	namespace
	{
		template<class Key, class Value> struct containerMap
		{
			containerMap(memoryArena arena) : cont(std::less<Key>(), arena) {}
			std::map<Key, Value, std::less<Key>, memoryArenaStd<>> cont;
			typedef typename std::map<Key, Value, std::less<Key>, memoryArenaStd<>>::iterator iter;
			typedef typename std::map<Key, Value, std::less<Key>, memoryArenaStd<>>::const_iterator citer;
		};

		struct inisection
		{
			inisection(memoryArena arena) : items(arena) {}
			containerMap<string, string> items;
		};

		class iniImpl : public iniClass
		{
		public:
			iniImpl(uintPtr memory) : pool(memory), arena(&pool), sections(arena) {}
			memoryArenaGrowing<memoryAllocatorPolicyPool<sizeof(templates::allocatorSizeMap<string, string>)>, memoryConcurrentPolicyNone> pool;
			memoryArena arena;
			containerMap<string, holder<inisection> > sections;
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
		containerMap<string, holder<inisection>>::citer i = impl->sections.cont.cbegin();
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
		return impl->sections.cont.find(section) != impl->sections.cont.end();
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
		containerMap<string, string>::citer i = impl->sections.cont[section]->items.cont.cbegin();
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
		containerMap<string, holder<inisection>>::citer i = impl->sections.cont.find(section);
		return i->second->items.cont.find(item) != i->second->items.cont.end();
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
		holder<fileClass> file = newFile(filename, fileMode(true, false, true));
		impl->clear();
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

	void iniClass::save(const string &filename) const
	{
		iniImpl *impl = (iniImpl*)this;
		holder<fileClass> file = newFile(filename, fileMode(false, true, true));
		containerMap <string, holder<inisection> >::citer i, e;
		for (i = impl->sections.cont.cbegin(), e = impl->sections.cont.cend(); i != e; i++)
		{
			file->writeLine(string() + "[" + i->first + "]");
			containerMap<string, string>::citer j, ee;
			for (j = i->second->items.cont.cbegin(), ee = i->second->items.cont.cend(); j != ee; j++)
				file->writeLine(string() + j->first + "=" + j->second);
		}
	}

	holder<iniClass> newIni(uintPtr memory)
	{
		return detail::systemArena().createImpl<iniClass, iniImpl>(memory);
	}
}