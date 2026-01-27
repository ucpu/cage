#include <algorithm>
#include <vector>

#include "database.h"

#include <cage-core/containerSerialization.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/string.h>

namespace cage
{
	Serializer &operator<<(Serializer &ser, const DatabaseAssetImpl &s)
	{
		ser << s.id << s.name << s.scheme << s.definitionPath;
		ser << s.fields << s.files << s.references;
		ser << s.corrupted;
		return ser;
	}

	Deserializer &operator>>(Deserializer &des, DatabaseAssetImpl &s)
	{
		des >> s.id >> s.name >> s.scheme >> s.definitionPath;
		des >> s.fields >> s.files >> s.references;
		des >> s.corrupted;
		return des;
	}

	Serializer &operator<<(Serializer &ser, const SchemeField &s)
	{
		ser << s.name;
		ser << s.type;
		ser << s.min;
		ser << s.max;
		ser << s.defaul;
		ser << s.values;
		return ser;
	}

	Deserializer &operator>>(Deserializer &des, SchemeField &s)
	{
		des >> s.name;
		des >> s.type;
		des >> s.min;
		des >> s.max;
		des >> s.defaul;
		des >> s.values;
		return des;
	}

	Serializer &operator<<(Serializer &ser, const Scheme &s)
	{
		ser << s.name << s.processor << s.schemeIndex;
		ser << s.schemeFields;
		return ser;
	}

	Deserializer &operator>>(Deserializer &des, Scheme &s)
	{
		des >> s.name >> s.processor >> s.schemeIndex;
		des >> s.schemeFields;
		return des;
	}

	namespace
	{
		constexpr String databaseBegin = "cage-asset-database-begin";
		constexpr String databaseVersion = "12";
		constexpr String databaseEnd = "cage-asset-database-end";
	}

	void AssetsDatabaseImpl::loadDatabase()
	{
		status = {};
		assets.clear();
		corruptedDefinitions.clear();
		lastModificationTime = {};

		if (!pathIsFile(config.databasePath))
		{
			CAGE_LOG(SeverityEnum::Info, "database", "database does not exist, nothing loaded");
			return;
		}

		const auto buf = readFile(config.databasePath)->readAll();
		Deserializer des(buf);
		String b;
		des >> b;
		if (b != databaseBegin)
			CAGE_THROW_ERROR(Exception, "invalid file format");
		des >> b;
		if (b != databaseVersion)
		{
			CAGE_LOG(SeverityEnum::Warning, "database", "assets database file version mismatch, database will not be loaded");
			return;
		}
		des >> lastModificationTime;
		des >> corruptedDefinitions;
		uint32 cnt = 0;
		des >> cnt;
		for (uint32 i = 0; i < cnt; i++)
		{
			Holder<DatabaseAssetImpl> ass = systemMemory().createHolder<DatabaseAssetImpl>();
			des >> *ass;
			assets[ass->name] = std::move(ass);
		}
		des >> b;
		if (b != databaseEnd)
			CAGE_THROW_ERROR(Exception, "invalid file format end");

		updateStatus();
	}

	void AssetsDatabaseImpl::saveDatabase()
	{
		// save database
		if (!config.databasePath.empty())
		{
			MemoryBuffer buf;
			Serializer ser(buf);
			ser << databaseBegin;
			ser << databaseVersion;
			ser << lastModificationTime;
			ser << corruptedDefinitions;
			ser << numeric_cast<uint32>(assets.size());
			for (const auto &it : assets)
				ser << *it.second;
			ser << databaseEnd;
			writeFile(config.databasePath)->write(buf);
		}

		// save list by hash
		if (!config.outputListByIds.empty())
		{
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(config.outputListByIds, fm);
			std::vector<std::pair<uint32, const DatabaseAssetImpl *>> items;
			for (const auto &it : assets)
			{
				const auto &ass = it.second;
				items.emplace_back(ass->id, +ass);
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
			f->writeLine("<id>       <asset name>                                                                                         <scheme>        <definition>");
			for (const auto &it : items)
			{
				const DatabaseAssetImpl &ass = *it.second;
				f->writeLine(Stringizer() + fill((Stringizer() + it.first).value, 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.definitionPath, 31));
			}
			f->close();
		}

		// save list by names
		if (!config.outputListByNames.empty())
		{
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(config.outputListByNames, fm);
			std::vector<std::pair<String, const DatabaseAssetImpl *>> items;
			for (const auto &it : assets)
			{
				const auto &ass = it.second;
				items.emplace_back(ass->name, +ass);
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
			f->writeLine("<id>       <asset name>                                                                                         <scheme>        <definition>");
			for (const auto &it : items)
			{
				const DatabaseAssetImpl &ass = *it.second;
				f->writeLine(Stringizer() + fill((Stringizer() + it.second->id).value, 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.definitionPath, 31));
			}
			f->close();
		}
	}
}
