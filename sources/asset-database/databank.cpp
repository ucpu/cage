#include <algorithm>
#include <vector>

#include "database.h"

#include <cage-core/containerSerialization.h>
#include <cage-core/files.h>
#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>

extern ConfigString configPathInput;
extern ConfigString configPathOutput;
extern ConfigString configPathIntermediate;
extern ConfigString configPathDatabase;
extern ConfigString configPathByHash;
extern ConfigString configPathByName;
extern ConfigString configPathInjectedNames;
extern ConfigBool configFromScratch;
extern ConfigBool configOutputArchive;
extern std::set<String, StringComparatorFast> configIgnoreExtensions;
extern std::set<String, StringComparatorFast> configIgnorePaths;
extern std::map<String, Holder<Scheme>, StringComparatorFast> schemes;

std::map<String, Holder<Asset>, StringComparatorFast> assets;
std::set<String, StringComparatorFast> corruptedDatabanks;
std::set<uint32> injectedNames;
PathLastChange lastModificationTime;

Serializer &operator<<(Serializer &ser, const Asset &s)
{
	ser << s.name << s.scheme << s.databank;
	ser << s.fields << s.files << s.references;
	ser << s.corrupted;
	return ser;
}

Deserializer &operator>>(Deserializer &des, Asset &s)
{
	des >> s.name >> s.scheme >> s.databank;
	des >> s.fields >> s.files >> s.references;
	des >> s.corrupted;
	return des;
}

uint32 Asset::outputPath() const
{
	return HashString(name);
}

namespace
{
	String convertAssetName(const String &name, const String &databank)
	{
		String detail;
		String p = name;
		{
			const uint32 sep = min(find(p, '?'), find(p, ';'));
			detail = subString(p, sep, m);
			p = subString(p, 0, sep);
		}
		if (p.empty())
			CAGE_THROW_ERROR(Exception, "empty path");
		if (pathIsAbs(p))
		{
			if (p[0] != '/')
				CAGE_THROW_ERROR(Exception, "absolute path with protocol");
			while (!p.empty() && p[0] == '/')
				p = subString(p, 1, m);
		}
		else
			p = pathJoin(pathExtractDirectory(databank), p);
		if (p.empty())
			CAGE_THROW_ERROR(Exception, "empty path");
		return p + detail;
	}
}

bool databankParse(const String &databank)
{
	Holder<Ini> ini = newIni();
	try
	{
		ini->importFile(pathJoin(configPathInput, databank));
	}
	catch (cage::Exception &)
	{
		CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "invalid databank ini file: " + databank);
		return false;
	}

	// load all sections
	uint32 errors = 0;
	for (const String &section : ini->sections())
	{
		// load scheme
		const String scheme = ini->getString(section, "scheme");
		if (scheme.empty())
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "undefined scheme in databank: " + databank + ", in section: " + section);
			errors++;
			continue;
		}
		if (schemes.count(scheme) == 0)
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "invalid scheme: " + scheme + ", in databank: " + databank + ", in section: " + section);
			errors++;
			continue;
		}
		const auto &sch = schemes[scheme];

		// find all assets
		for (const String &assItem : ini->items(section))
		{
			if (!isDigitsOnly(assItem))
				continue; // not an asset

			Holder<Asset> ass = systemMemory().createHolder<Asset>();
			ass->scheme = scheme;
			ass->databank = databank;
			ass->name = ini->getString(section, assItem);
			try
			{
				ass->name = convertAssetName(ass->name, databank);
			}
			catch (const cage::Exception &)
			{
				errors++;
				continue;
			}

			// check for duplicate asset name
			// (in case of multiple databanks in one folder)
			if (assets.count(ass->name) > 0)
			{
				const auto &ass2 = assets[ass->name];
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "duplicate asset name: " + ass->name + ", in databank: " + databank + ", in section: " + section + ", with asset in databank: " + ass2->databank);
				errors++;
				continue;
			}

			// load asset properties
			for (const auto &propit : sch->schemeFields)
			{
				const String prop = propit.first;
				if (ini->itemExists(section, prop))
					ass->fields[prop] = ini->getString(section, prop);
			}

			assets.emplace(ass->name, std::move(ass));
		}
	}

	// check unused
	if (errors == 0)
	{
		String s, t, v;
		if (ini->anyUnused(s, t, v))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "unused property/asset: " + t + " = " + v + ", in databank: " + databank + ", in section: " + s);
			errors++;
		}
	}

	return errors == 0;
}

namespace
{
	constexpr String databaseBegin = "cage-asset-database-begin";
	constexpr String databaseVersion = "11";
	constexpr String databaseEnd = "cage-asset-database-end";
}

void databanksLoad()
{
	assets.clear();
	corruptedDatabanks.clear();
	injectedNames.clear();
	lastModificationTime = {};

	if (!((String)configPathInjectedNames).empty())
	{
		Holder<File> f = readFile(configPathInjectedNames);
		String l;
		f->readLine(l); // skip the header
		while (f->readLine(l))
			if (!l.empty())
				injectedNames.insert(toUint32(split(l)));
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "injected " + injectedNames.size() + " names");
	}

	if (configFromScratch)
	{
		CAGE_LOG(SeverityEnum::Info, "database", "'from scratch' was defined, ignoring previous cache");
		pathRemove(configPathDatabase);
		return;
	}

	CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loading database cache: " + (String)configPathDatabase);
	if (!pathIsFile(configPathDatabase))
	{
		CAGE_LOG(SeverityEnum::Info, "database", "database cache does not exist, starting from scratch");
		return;
	}

	const auto buf = readFile(configPathDatabase)->readAll();
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
	des >> corruptedDatabanks;
	uint32 cnt = 0;
	des >> cnt;
	for (uint32 i = 0; i < cnt; i++)
	{
		Holder<Asset> ass = systemMemory().createHolder<Asset>();
		des >> *ass;
		assets[ass->name] = std::move(ass);
	}
	des >> b;
	if (b != databaseEnd)
		CAGE_THROW_ERROR(Exception, "wrong file end");
	CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loaded " + assets.size() + " asset entries");
}

void databanksSave()
{
	// save database
	if (!((String)configPathDatabase).empty())
	{
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "saving database cache: " + (String)configPathDatabase);
		MemoryBuffer buf;
		Serializer ser(buf);
		ser << databaseBegin;
		ser << databaseVersion;
		ser << lastModificationTime;
		ser << corruptedDatabanks;
		ser << numeric_cast<uint32>(assets.size());
		for (const auto &it : assets)
			ser << *it.second;
		ser << databaseEnd;
		writeFile(configPathDatabase)->write(buf);
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "saved " + assets.size() + " asset entries");
	}

	// save list by hash
	if (!((String)configPathByHash).empty())
	{
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> f = newFile(configPathByHash, fm);
		std::vector<std::pair<uint32, const Asset *>> items;
		for (const auto &it : assets)
		{
			const auto &ass = it.second;
			items.emplace_back(ass->outputPath(), +ass);
		}
		std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
		f->writeLine("<hash>     <asset name>                                                                                         <scheme>        <databank>");
		for (const auto &it : items)
		{
			const String hash = Stringizer() + it.first;
			const Asset &ass = *it.second;
			f->writeLine(Stringizer() + fill(hash, 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.databank, 31));
		}
	}

	// save list by names
	if (!((String)configPathByName).empty())
	{
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> f = newFile(configPathByName, fm);
		std::vector<std::pair<String, const Asset *>> items;
		for (const auto &it : assets)
		{
			const auto &ass = it.second;
			items.emplace_back(ass->name, +ass);
		}
		std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
		f->writeLine("<hash>     <asset name>                                                                                         <scheme>        <databank>");
		for (const auto &it : items)
		{
			const Asset &ass = *it.second;
			const String hash = Stringizer() + ass.outputPath();
			f->writeLine(Stringizer() + fill(hash, 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.databank, 31));
		}
	}
}

bool isNameDatabank(const String &name)
{
	return isPattern(name, "", "", ".assets");
}

bool isNameIgnored(const String &name)
{
	for (const String &it : configIgnoreExtensions)
	{
		if (isPattern(name, "", "", it))
			return true;
	}
	for (const String &it : configIgnorePaths)
	{
		if (isPattern(name, it, "", ""))
			return true;
	}
	return false;
}

namespace
{
	void findFiles(std::map<String, PathLastChange, StringComparatorFast> &files, const String &path)
	{
		const String pth = pathJoin(configPathInput, path);
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "checking path: " + pth);
		const auto list = pathListDirectory(pth);
		for (const String &p : list)
		{
			const String n = pathToRel(p, configPathInput);
			if (!isNameIgnored(n))
			{
				if (pathIsDirectory(p))
					findFiles(files, n);
				else
					files[n] = pathLastChange(p);
			}
		}
	}
}

std::map<String, PathLastChange, StringComparatorFast> findFiles()
{
	std::map<String, PathLastChange, StringComparatorFast> files;
	findFiles(files, "");
	return files;
}

void checkOutputDir()
{
	const PathTypeFlags t = pathType(configPathOutput);
	if (configOutputArchive && any(t & PathTypeFlags::NotFound))
		return pathCreateArchiveCarch(configPathOutput);
	if (any(t & PathTypeFlags::Archive))
		return;
	// the output is not an archive, output to it directly
	configPathIntermediate = "";
}
