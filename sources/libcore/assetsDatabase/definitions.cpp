#include <unordered_map>

#include "database.h"

#include <cage-core/files.h>
#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/math.h>

namespace cage
{
	AssetsDatabaseImpl::AssetsDatabaseImpl(const AssetsDatabaseCreateConfig &config_) : config(config_)
	{
		config.databasePath = pathSimplify(config.databasePath);
		config.intermediatePath = pathSimplify(config.intermediatePath);
		config.inputPath = pathSimplify(config.inputPath);
		config.outputListByIds = pathSimplify(config.outputListByIds);
		config.outputListByNames = pathSimplify(config.outputListByNames);
		config.outputPath = pathSimplify(config.outputPath);
	}

	void AssetsDatabaseImpl::updateStatus()
	{
		status = {};

		status.corruptedDefinitions = corruptedDefinitions.size();
		status.totalAssets = assets.size();
		for (const auto &it : assets)
		{
			status.corruptedAssets += it.second->corrupted;
			status.modifiedAssets += it.second->modified;
		}
		status.missingReferences = assetsMissingReferences().size();
		status.ok = status.corruptedDefinitions == 0 && status.corruptedAssets == 0 && status.missingReferences == 0;
	}

	namespace
	{
		String convertAssetName(const String &name, const String &definitionPath)
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
				p = pathJoin(pathExtractDirectory(definitionPath), p);
			if (p.empty())
				CAGE_THROW_ERROR(Exception, "empty path");
			return p + detail;
		}
	}

	void AssetsDatabaseImpl::parseDefinition(const String &path)
	{
		Holder<Ini> ini = newIni();
		ini->importFile(pathJoin(config.inputPath, path));

		// load all sections
		uint32 errors = 0;
		for (const String &section : ini->sections())
		{
			// load scheme
			const String scheme = ini->getString(section, "scheme");
			if (scheme.empty())
			{
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "undefined scheme in definition: " + path + ", in section: " + section);
				errors++;
				continue;
			}
			if (schemes.count(scheme) == 0)
			{
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "invalid scheme: " + scheme + ", in definition: " + path + ", in section: " + section);
				errors++;
				continue;
			}
			const auto &sch = schemes[scheme];

			// find all assets
			for (const String &assItem : ini->items(section))
			{
				if (!isDigitsOnly(assItem))
					continue; // not an asset

				Holder<DatabaseAssetImpl> ass = systemMemory().createHolder<DatabaseAssetImpl>();
				ass->scheme = scheme;
				ass->definitionPath = path;
				ass->name = ini->getString(section, assItem);
				try
				{
					ass->name = convertAssetName(ass->name, path);
				}
				catch (const cage::Exception &)
				{
					errors++;
					continue;
				}
				ass->id = HashString(ass->name);

				// check for duplicate asset name
				// (in case of multiple databanks in one folder)
				if (assets.count(ass->name) > 0)
				{
					const auto &ass2 = assets[ass->name];
					CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "duplicate asset name: " + ass->name + ", in definition: " + path + ", in section: " + section + ", with asset in definition: " + ass2->definitionPath);
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
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "unused property/asset: " + t + " = " + v + ", in definition: " + path + ", in section: " + s);
				errors++;
			}
		}

		if (errors)
		{
			CAGE_LOG_THROW(Stringizer() + "definition path: " + path);
			CAGE_THROW_ERROR(Exception, "errors while parsing definition");
		}
	}

	namespace
	{
		void findFiles(AssetsDatabaseImpl *impl, std::map<String, PathLastChange, StringComparatorFast> &files, const String &path)
		{
			const String pth = pathJoin(impl->config.inputPath, path);
			CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "checking path: " + pth);
			const auto list = pathListDirectory(pth);
			for (const String &p : list)
			{
				const String n = pathToRel(p, impl->config.inputPath);
				if (!impl->isIgnored(n))
				{
					if (pathIsDirectory(p))
						findFiles(impl, files, n);
					else
						files[n] = pathLastChange(p);
				}
			}
		}

		bool isNameDatabank(const String &name)
		{
			return isPattern(name, "", "", ".assets");
		}
	}

	void AssetsDatabaseImpl::detectChanges()
	{
		status = {};

		std::map<String, PathLastChange, StringComparatorFast> files;
		findFiles(this, files, "");

		for (auto asIt = assets.begin(); asIt != assets.end();)
		{
			DatabaseAssetImpl &ass = *asIt->second;

			// check for deleted, modified or corrupted definitions
			if (files.count(ass.definitionPath) == 0 || files.at(ass.definitionPath) > lastModificationTime || corruptedDefinitions.count(ass.definitionPath) > 0)
			{
				asIt = assets.erase(asIt);
				continue;
			}

			// check for deleted or modified files
			for (const String &f : ass.files)
			{
				if (files.count(f) == 0 || files.at(f) > lastModificationTime)
					ass.corrupted = true;
			}

			asIt++;
		}

		// find new or modified definitions
		std::set<String, StringComparatorFast> corruptedDbsCopy;
		std::swap(corruptedDbsCopy, corruptedDefinitions);
		for (const auto &f : files)
		{
			if (!isNameDatabank(f.first))
				continue;
			if (f.second <= lastModificationTime && corruptedDbsCopy.count(f.first) == 0)
				continue;
			try
			{
				parseDefinition(f.first);
			}
			catch (...)
			{
				corruptedDefinitions.insert(f.first);
			}
		}

		{ // update modification time
			lastModificationTime = {};
			for (const auto &f : files)
				lastModificationTime.data = max(lastModificationTime.data, f.second.data);
		}

		{ // check for output hash collisions
			std::unordered_map<uint32, DatabaseAssetImpl *> outputHashes;
			for (const auto &it : assets)
			{
				DatabaseAssetImpl &ass = *it.second;
				if (outputHashes.count(ass.id) > 0)
				{
					auto &ass2 = *outputHashes[ass.id];
					CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset: " + ass.name + ", in definition: " + ass.definitionPath + ", has name hash collision with asset: " + ass2.name + ", in definition: " + ass2.definitionPath);
					ass.corrupted = true;
					ass2.corrupted = true;
				}
				else
					outputHashes[ass.id] = +it.second;
			}
		}

		updateStatus();
	}
}
