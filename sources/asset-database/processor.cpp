#include <cage-core/memoryBuffer.h>
#include <cage-core/process.h>
#include <cage-core/config.h>
#include <cage-core/tasks.h>
#include <cage-core/debug.h>
#include <cage-core/math.h>
#include <cage-core/ini.h>

#include "database.h"

#include <algorithm>
#include <vector>

#define CAGE_THROW_WARNING(EXCEPTION, ...) { EXCEPTION e(__FUNCTION__, __FILE__, __LINE__, ::cage::SeverityEnum::Warning, __VA_ARGS__); e.makeLog(); throw e; }

namespace
{
	const String databaseBegin = "cage-asset-database-begin";
	const String databaseVersion = "10";
	const String databaseEnd = "cage-asset-database-end";

	bool verdictValue = false;

	struct Databank
	{
		String name;

		bool operator < (const Databank &other) const
		{
			return StringComparatorFast()(name, other.name);
		}
	};

	uint64 timestamp;
	HolderSet<Scheme> schemes;
	HolderSet<Asset> assets;
	HolderSet<Databank> corruptedDatabanks;

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

	bool parseDatabank(const String &path)
	{
		Holder<Ini> ini = newIni();
		try
		{
			ini->importFile(pathJoin(configPathInput, path));
		}
		catch (cage::Exception &)
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "invalid ini file in databank '" + path + "'");
			return false;
		}

		// load all sections
		uint32 errors = 0;
		for (const String &section : ini->sections())
		{
			// load scheme
			String scheme = ini->getString(section, "scheme");
			if (scheme.empty())
			{
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "undefined scheme in databank '" + path + "' in section '" + section + "'");
				errors++;
				continue;
			}
			Scheme *sch = schemes.retrieve(scheme);
			if (!sch)
			{
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "invalid scheme '" + scheme + "' in databank '" + path + "' in section '" + section + "'");
				errors++;
				continue;
			}

			const auto items = ini->items(section);

			// find invalid properties
			bool propertiesOk = true;
			for (const String &prop : items)
			{
				if (isDigitsOnly(prop) || prop == "scheme")
					continue;
				if (!sch->schemeFields.exists(prop))
				{
					CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "unknown property '" + prop + "' (value '" + ini->getString(section, prop) + "') in databank '" + path + "' in section '" + section + "'");
					propertiesOk = false;
				}
			}
			if (!propertiesOk)
			{
				errors++;
				continue;
			}

			// find all assets
			for (const String &assItem : items)
			{
				if (!isDigitsOnly(assItem))
					continue; // not an asset

				bool ok = true;
				Asset ass;
				ass.scheme = scheme;
				ass.databank = path;
				ass.name = ini->getString(section, assItem);
				try
				{
					ass.name = convertAssetName(ass.name, path);
				}
				catch (const cage::Exception &)
				{
					ok = false;
				}

				// check for duplicate asset name
				// (in case of multiple databanks in one folder)
				if (assets.exists(ass.name))
				{
					CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "duplicate asset name '" + ass.name + "' in databank '" + path + "' in section '" + section + "'");
					Asset &ass2 = *const_cast<Asset*>(assets.retrieve(ass.name));
					CAGE_LOG(SeverityEnum::Note, "database", Stringizer() + "with asset in databank '" + ass2.databank + "'");
					ass2.corrupted = true;
					ok = false;
				}

				// check for hash collisions
				for (const auto &it : assets)
				{
					Asset &ass2 = *it;
					if (ass2.outputPath() == ass.outputPath())
					{
						CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset output path collision '" + ass.name + "' in databank '" + path + "' in section '" + section + "'");
						CAGE_LOG(SeverityEnum::Note, "database", Stringizer() + "with '" + ass2.name + "' in databank '" + ass2.databank + "'");
						ass2.corrupted = true;
						ok = false;
					}
				}

				// load asset properties
				if (ok)
				{
					for (const String &prop : items)
					{
						if (isDigitsOnly(prop) || prop == "scheme")
							continue;
						CAGE_ASSERT(sch->schemeFields.exists(prop));
						ass.fields[prop] = ini->getString(section, prop);
					}
				}
				else
				{
					errors++;
					ass.corrupted = true;
				}

				assets.insert(std::move(ass));
			}
		}

		// check unused
		if (errors == 0)
		{
			String s, t, v;
			if (ini->anyUnused(s, t, v))
			{
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "unused property/asset '" + t + "' (value '" + v + "') in databank '" + path + "' in section '" + s + "'");
				errors++;
			}
		}

		return errors == 0;
	}

	void processAsset(Asset &ass)
	{
		CAGE_LOG(SeverityEnum::Info, "asset", ass.name);
		ass.corrupted = true;
		ass.needNotify = true;
		ass.files.clear();
		ass.references.clear();
		ass.aliasName = "";
		Scheme *scheme = schemes.retrieve(ass.scheme);
		CAGE_ASSERT(scheme);
		detail::OverrideBreakpoint overrideBreakpoint;
		try
		{
			if (!scheme->applyOnAsset(ass))
				CAGE_THROW_WARNING(Exception, "asset has invalid configuration");

			Holder<Process> prg = newProcess(scheme->processor);
			prg->writeLine(Stringizer() + "inputDirectory=" + pathToAbs(configPathInput)); // inputDirectory
			prg->writeLine(Stringizer() + "inputName=" + ass.name); // inputName
			prg->writeLine(Stringizer() + "outputDirectory=" + pathToAbs(String(configPathIntermediate).empty() ? configPathOutput : configPathIntermediate)); // outputDirectory
			prg->writeLine(Stringizer() + "outputName=" + ass.outputPath()); // outputName
			prg->writeLine(Stringizer() + "schemeIndex=" + scheme->schemeIndex); // schemeIndex
			for (const auto &it : ass.fields)
				prg->writeLine(Stringizer() + it.first + "=" + it.second);
			prg->writeLine("cage-end");

			bool begin = false, end = false;
			while (true)
			{
				String line = prg->readLine();
				if (line.empty() || line == "cage-error")
				{
					CAGE_THROW_WARNING(Exception, "processing thrown an error");
				}
				else if (line == "cage-begin")
				{
					if (end || begin)
						CAGE_THROW_WARNING(Exception, "unexpected cage-begin");
					begin = true;
				}
				else if (line == "cage-end")
				{
					if (end || !begin)
						CAGE_THROW_WARNING(Exception, "unexpected cage-end");
					end = true;
					break;
				}
				else if (begin && !end)
				{
					String param = trim(split(line, "="));
					line = trim(line);
					if (param == "use")
					{
						if (pathIsAbs(line))
						{
							CAGE_LOG_THROW(Stringizer() + "path: '" + line + "'");
							CAGE_THROW_WARNING(Exception, "assets use path must be relative");
						}
						if (!pathIsFile(pathJoin(pathToAbs(configPathInput), line)))
						{
							CAGE_LOG_THROW(Stringizer() + "path: '" + line + "'");
							CAGE_THROW_WARNING(Exception, "assets use path does not exist");
						}
						ass.files.insert(line);
					}
					else if (param == "ref")
						ass.references.insert(line);
					else if (param == "alias")
					{
						if (ass.aliasName.empty())
							ass.aliasName = line;
						else
						{
							CAGE_LOG_THROW(Stringizer() + "previous: '" + ass.aliasName + "'");
							CAGE_LOG_THROW(Stringizer() + "current: '" + line + "'");
							CAGE_THROW_WARNING(Exception, "assets alias name cannot be overridden");
						}
					}
					else
					{
						CAGE_LOG_THROW(Stringizer() + "parameter: " + param);
						CAGE_LOG_THROW(Stringizer() + "value: " + line);
						CAGE_THROW_WARNING(Exception, "unknown parameter name");
					}
				}
				else
					CAGE_LOG(SeverityEnum::Note, "processor", line);
			}

			int ret = prg->wait();
			if (ret != 0)
				CAGE_THROW_WARNING(SystemError, "process returned error code", ret);

			if (ass.files.empty())
				CAGE_THROW_WARNING(Exception, "asset reported no used files");

			ass.corrupted = false;
		}
		catch (const Exception &e)
		{
			if (e.severity >= SeverityEnum::Error)
				throw;
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "processing asset '" + ass.name + "' failed");
		}
		catch (...)
		{
			detail::logCurrentCaughtException();
			CAGE_THROW_ERROR(Exception, "unknown exception during asset processing");
		}
	}

	void load()
	{
		if (!pathIsFile(configPathDatabase))
			return;
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loading database cache: '" + (String)configPathDatabase + "'");
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
		des >> timestamp;
		des >> corruptedDatabanks;
		des >> assets;
		des >> b;
		if (b != databaseEnd)
			CAGE_THROW_ERROR(Exception, "wrong file end");
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loaded " + assets.size() + " asset entries");
	}

	void save()
	{
		// save database
		if (!((String)configPathDatabase).empty())
		{
			CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "saving database cache: '" + (String)configPathDatabase + "'");
			MemoryBuffer buf;
			Serializer ser(buf);
			ser << databaseBegin;
			ser << databaseVersion;
			ser << timestamp;
			ser << corruptedDatabanks;
			ser << assets;
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
			std::vector<std::pair<String, const Asset*>> items;
			for (const auto &it : assets)
			{
				const Asset &ass = *it;
				items.push_back(std::pair<String, const Asset*>(ass.outputPath(), &ass));
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			f->writeLine("<hash>     <asset name>                                                                                         <scheme>        <databank>");
			for (const auto &it : items)
			{
				const Asset &ass = *it.second;
				f->writeLine(Stringizer() + fill(it.first, 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.databank, 31));
			}
		}

		// save list by names
		if (!((String)configPathByName).empty())
		{
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(configPathByName, fm);
			std::vector<std::pair<String, const Asset*>> items;
			for (const auto &it : assets)
			{
				const Asset &ass = *it;
				items.push_back(std::pair<String, const Asset*>(ass.name, &ass));
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			f->writeLine("<hash>     <asset name>                                                                                         <scheme>        <databank>");
			for (const auto &it : items)
			{
				const Asset &ass = *it.second;
				f->writeLine(Stringizer() + fill(it.second->outputPath(), 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.databank, 31));
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

	using FilesMap = std::map<String, uint64, StringComparatorFast>;
	FilesMap files;

	void findFiles(const String &path)
	{
		String pth = pathJoin(configPathInput, path);
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "checking path '" + pth + "'");
		Holder<DirectoryList> d = newDirectoryList(pth);
		while (d->valid())
		{
			String p = pathJoin(path, d->name());
			if (d->isDirectory())
				findFiles(p);
			else if (!isNameIgnored(p))
			{
				try
				{
					detail::OverrideBreakpoint o;
					uint64 lt = d->lastChange();
					files[p] = lt;
				}
				catch (...)
				{
					// do nothing
				}
			}
			d->next();
		}
	}

	void moveIntermediateFiles()
	{
		Holder<DirectoryList> listIn = newDirectoryList(configPathIntermediate);
		Holder<DirectoryList> listOut = newDirectoryList(configPathOutput); // keep the archive open until all files are written (this significantly speeds up the moving process, but it causes the process to keep all the files in memory)
		uint64 movedSize = 0;
		while (listIn->valid())
		{
			if (movedSize > configArchiveWriteThreshold)
			{
				listOut.clear(); // close the archive
				listOut = newDirectoryList(configPathOutput); // reopen the archive
				movedSize = 0;
			}
			const String f = pathJoin(configPathIntermediate, listIn->name());
			const String t = pathJoin(configPathOutput, listIn->name());
			movedSize += readFile(f)->size();
			pathMove(f, t);
			listIn->next();
		}
		pathRemove(configPathIntermediate);
	}

	void sendAllNotifications()
	{
		for (auto &it : assets)
		{
			if (it->needNotify)
			{
				CAGE_ASSERT(!it->corrupted);
				notifierNotify(it->name);
				const_cast<Asset*>(it.get())->needNotify = false;
			}
		}
	}

	void dispatchAssetProcessing(Asset *const &ass)
	{
		try
		{
			processAsset(*ass);
		}
		catch (const cage::Exception &)
		{
			// do nothing
		}
		catch (...)
		{
			detail::logCurrentCaughtException();
			CAGE_LOG(SeverityEnum::Error, "exception", "caught unknown exception in asset processing thread");
		}
	}

	void checkAssets()
	{
		CAGE_LOG(SeverityEnum::Info, "database", "looking for assets to process");
		if (!String(configPathIntermediate).empty())
			pathRemove(configPathIntermediate);
		verdictValue = false;
		files.clear();
		findFiles("");
		HolderSet<Databank> corruptedDbsCopy;
		std::swap(corruptedDbsCopy, corruptedDatabanks);
		corruptedDatabanks.clear();
		uint32 countBadDatabanks = 0;
		uint32 countCorrupted = 0;
		uint32 countMissingReferences = 0;

		for (auto asIt = assets.begin(); asIt != assets.end();)
		{
			Asset &ass = **asIt;

			// check for deleted, modified or corrupted databank
			if (files.count(ass.databank) == 0 || corruptedDbsCopy.count(ass.databank) != 0 || files[ass.databank] > timestamp)
			{
				asIt = assets.erase(asIt);
				continue;
			}

			// check for deleted or modified files
			for (const String &f : ass.files)
			{
				if (files.count(f) == 0 || files[f] > timestamp)
					ass.corrupted = true;
			}

			asIt++;
		}

		// find new or modified databanks
		uint64 newestFile = 0;
		for (const auto &f : files)
		{
			newestFile = std::max(newestFile, f.second);
			if (isNameDatabank(f.first))
			{
				const bool wasCorrupted = corruptedDbsCopy.find(f.first) != corruptedDbsCopy.end();
				if (f.second > timestamp || wasCorrupted)
				{
					const bool corrupted = !parseDatabank(f.first);
					if (corrupted)
						corruptedDatabanks.insert(Databank{ f.first });
					countBadDatabanks += corrupted;
				}
			}
		}

		StringSet outputHashes;
		for (const auto &it : assets)
			outputHashes.insert(it->outputPath());

		{ // reprocess assets
			std::vector<Asset *> asses;
			asses.reserve(assets.size());
			for (const auto &it : assets)
				if (it->corrupted)
					asses.push_back(+it);
			tasksRunBlocking<Asset *const>("processing", Delegate<void(Asset *const &)>().bind<&dispatchAssetProcessing>(), asses);
		}

		for (const auto &it : assets)
		{
			Asset &ass = *it;

			// check alias name collisions
			if (!ass.aliasName.empty() && outputHashes.find(ass.aliasPath()) != outputHashes.end())
			{
				ass.corrupted = true;
				CAGE_LOG(SeverityEnum::Warning, "database", Stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' with alias name '" + ass.aliasName + "' collides with another asset");
			}

			// warn about missing references
			bool anyMissing = false;
			for (const String &it : ass.references)
			{
				if (!assets.exists(it))
				{
					anyMissing = true;
					CAGE_LOG(SeverityEnum::Warning, "database", Stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' is missing reference '" + it + "'");
				}
			}
			if (anyMissing)
				countMissingReferences++;

			// count corrupted
			if (ass.corrupted)
				countCorrupted++;
		}

		// finalize
		timestamp = newestFile;
		if (!String(configPathIntermediate).empty())
			moveIntermediateFiles();
		save();
		if (countBadDatabanks || countCorrupted || countMissingReferences)
		{
			CAGE_LOG(SeverityEnum::Warning, "verdict", Stringizer() +
				countBadDatabanks + " corrupted databanks, " +
				countCorrupted + " corrupted assets, " +
				countMissingReferences + " missing references");
		}
		else
		{
			sendAllNotifications();
			CAGE_LOG(SeverityEnum::Info, "verdict", "ok");
			verdictValue = true;
		}
	}

	void loadSchemesDirectory(const String &dir)
	{
		String realpath = pathJoin(configPathSchemes, dir);
		Holder<DirectoryList> lst = newDirectoryList(realpath);
		for (; lst->valid(); lst->next())
		{
			String name = pathJoin(dir, lst->name());
			if (lst->isDirectory())
			{
				loadSchemesDirectory(name);
				continue;
			}
			if (!isPattern(name, "", "", ".scheme"))
				continue;
			Scheme s;
			s.name = subString(name, 0, name.length() - 7);
			CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loading scheme '" + s.name + "'");
			Holder<Ini> ini = newIni();
			ini->importFile(pathJoin(configPathSchemes, name));
			s.parse(ini.get());
			schemes.insert(std::move(s));
		}
	}

	void checkOutputDir()
	{
		PathTypeFlags t = pathType(configPathOutput);
		if (configOutputArchive && (t & PathTypeFlags::NotFound) == PathTypeFlags::NotFound)
			return pathCreateArchive(configPathOutput);
		if ((t & PathTypeFlags::Archive) == PathTypeFlags::Archive)
			return;
		// the output is not an archive, output to it directly
		configPathIntermediate = "";
	}
}

void start()
{
	// load schemes
	loadSchemesDirectory("");
	CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loaded " + schemes.size() + " schemes");

	// load
	timestamp = 0;
	if (configFromScratch)
		CAGE_LOG(SeverityEnum::Info, "database", "'from scratch' was defined. Previous cache is ignored.");
	else
		load();

	// find changes
	checkOutputDir();
	checkAssets();
}

void listen()
{
	CAGE_LOG(SeverityEnum::Info, "database", "initializing listening for changes");
	notifierInitialize(configNotifierPort);
	Holder<FilesystemWatcher> changes = newFilesystemWatcher();
	changes->registerPath(configPathInput);
	uint32 cycles = 0;
	bool changed = false;
	while (true)
	{
		notifierAccept();
		{
			String path = changes->waitForChange(1000 * 200);
			if (!path.empty())
			{
				path = pathToRel(path, configPathInput);
				if (isNameIgnored(path))
					continue;
				cycles = 0;
				changed = true;
			}
		}
		if (!changed)
			continue;
		if (cycles >= 3)
		{
			checkAssets();
			changed = false;
			cycles = 0;
		}
		else
			cycles++;
	}
}

bool verdict()
{
	return verdictValue;
}
