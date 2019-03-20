#include <algorithm>
#include <vector>
#include <set>
#include <map>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/filesystem.h>
#include <cage-core/concurrent.h>
#include <cage-core/ini.h>
#include <cage-core/program.h>
#include <cage-core/threadPool.h>

using namespace cage;

#include "utilities.h"
#include "holderSet.h"
#include "processor.h"
#include "config.h"
#include "asset.h"
#include "scheme.h"
#include "notifier.h"

namespace
{
	const string databaseBegin = "cage-asset-database-begin";
	const string databaseVersion = "8";
	const string databaseEnd = "cage-asset-database-end";

	bool verdictValue = false;

	struct databankStruct
	{
		string name;
		databankStruct(const string &s = "") : name(s) {};

		void load(fileClass *f)
		{
			read(f, name);
		}

		void save(fileClass *f) const
		{
			write(f, name);
		}

		bool operator < (const databankStruct &other) const
		{
			return stringCompareFast(name, other.name);
		}
	};

	uint64 timestamp;
	holderSet<schemeStruct> schemes;
	holderSet<assetStruct> assets;
	holderSet<databankStruct> corruptedDatabanks;

	bool parseDatabank(const string &path)
	{
		holder<iniClass> ini = newIni();
		try
		{
			ini->load(pathJoin(configPathInput, path));
		}
		catch (cage::exception &)
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "invalid ini file in databank '" + path + "'");
			return false;
		}
		uint32 errors = 0;
		for (uint32 sectionIndex = 0, sectionIndexEnd = ini->sectionCount(); sectionIndex != sectionIndexEnd; sectionIndex++)
		{
			// load assets group properties
			string section = ini->section(sectionIndex);
			string scheme = ini->getString(section, "scheme");
			if (scheme.empty())
			{
				CAGE_LOG(severityEnum::Error, "database", string() + "undefined scheme in databank '" + path + "' in section '" + section + "'");
				errors++;
				continue;
			}
			schemeStruct *sch = schemes.retrieve(scheme);
			if (!sch)
			{
				CAGE_LOG(severityEnum::Error, "database", string() + "invalid scheme '" + scheme + "' in databank '" + path + "' in section '" + section + "'");
				errors++;
				continue;
			}

			// find invalid properties
			for (uint32 i = 0, e = ini->itemCount(section); i != e; i++)
			{
				string item = ini->item(section, i);
				if (item.isDigitsOnly() || item == "scheme")
					continue;
				if (sch->schemeFields.find(item) == sch->schemeFields.end())
				{
					CAGE_LOG(severityEnum::Error, "database", string() + "invalid property '" + item + "' (value '" + ini->getString(section, item) + "') in databank '" + path + "' in section '" + section + "'");
					errors++;
				}
			}

			// find all assets in the group
			for (uint32 index = 0; ini->itemExists(section, string(index)); index++)
			{
				assetStruct ass;
				ass.scheme = scheme;
				ass.databank = path;
				ass.name = ini->getString(section, string(index));
				ass.name = pathJoin(pathExtractPath(ass.databank), ass.name);
				bool ok = true;
				if (assets.exists(ass.name))
				{
					CAGE_LOG(severityEnum::Error, "database", string() + "duplicate asset name '" + ass.name + "' in databank '" + path + "' in section '" + section + "'");
					assetStruct &ass2 = *const_cast<assetStruct*>(assets.retrieve(ass.name));
					CAGE_LOG(severityEnum::Note, "database", string() + "with asset in databank '" + ass2.databank + "'");
					ass2.corrupted = true;
					ok = false;
				}
				for (holderSet<assetStruct>::iterator asIt = assets.begin(), asEt = assets.end(); asIt != asEt; asIt++)
				{
					assetStruct &ass2 = *const_cast<assetStruct*>(asIt->get());
					if (ass2.outputPath() == ass.outputPath())
					{
						CAGE_LOG(severityEnum::Error, "database", string() + "asset output path collision '" + ass.name + "' in databank '" + path + "' in section '" + section + "'");
						CAGE_LOG(severityEnum::Note, "database", string() + "with '" + ass2.name + "' in databank '" + ass2.databank + "'");
						ass2.corrupted = true;
						ok = false;
					}
				}
				if (!ok)
				{
					errors++;
					ass.corrupted = true;
				}
				for (uint32 i = 0, e = ini->itemCount(section); i != e; i++)
				{
					string item = ini->item(section, i);
					if (item.isDigitsOnly() || item == "scheme")
						continue;
					ass.fields[item] = ini->getString(section, item);
				}
				assets.insert(templates::move(ass));
			}
		}
		return errors == 0;
	}

	void processAsset(assetStruct &ass)
	{
		detail::overrideBreakpoint overrideBreakpoint;
		CAGE_LOG(severityEnum::Info, "asset", ass.name);
		ass.corrupted = true;
		ass.files.clear();
		ass.references.clear();
		ass.internationalizedName = "";
		schemeStruct *scheme = schemes.retrieve(ass.scheme);
		CAGE_ASSERT_RUNTIME(scheme, "asset has invalid scheme");
		try
		{
			if (!scheme->applyOnAsset(ass))
				CAGE_THROW_WARNING(exception, "asset has invalid configuration");

			holder<programClass> prg = newProgram(scheme->processor);
			prg->writeLine(pathToAbs(configPathInput)); // inputDirectory
			prg->writeLine(ass.name); // inputName
			prg->writeLine(pathToAbs(configPathOutput)); // outputDirectory
			prg->writeLine(ass.outputPath()); // outputName
			prg->writeLine(ass.databank); // assetPath
			prg->writeLine(pathJoin(configPathScheme, ass.scheme)); // schemePath
			prg->writeLine(string(scheme->schemeIndex)); // schemeIndex
			for (stringMap::const_iterator it = ass.fields.begin(), et = ass.fields.end(); it != et; it++)
				prg->writeLine(string() + it->first + "=" + it->second);
			prg->writeLine("cage-end");

			bool begin = false, end = false;
			while (true)
			{
				string line = prg->readLine();
				if (line.empty() || line == "cage-error")
				{
					CAGE_THROW_WARNING(exception, "processing thrown an error");
				}
				else if (line == "cage-begin")
				{
					if (end || begin)
						CAGE_THROW_WARNING(exception, "unexpected cage-begin");
					begin = true;
				}
				else if (line == "cage-end")
				{
					if (end || !begin)
						CAGE_THROW_WARNING(exception, "unexpected cage-end");
					end = true;
					break;
				}
				else if (begin && !end)
				{
					string param = line.split("=").trim();
					line = line.trim();
					if (param == "use")
						ass.files.insert(line);
					else if (param == "ref")
						ass.references.insert(line);
					else if (param == "internationalized")
					{
						if (ass.internationalizedName.empty())
							ass.internationalizedName = line;
						else
							CAGE_THROW_WARNING(exception, "assets internationalized name cannot be overriden");
					}
					else
					{
						CAGE_LOG(severityEnum::Note, "exception", string("parameter: ") + param);
						CAGE_LOG(severityEnum::Note, "exception", string("value: ") + line);
						CAGE_THROW_WARNING(exception, "unknown parameter name");
					}
				}
				else
					CAGE_LOG(severityEnum::Note, "processor", line);
			}

			int ret = prg->wait();
			if (ret != 0)
				CAGE_THROW_WARNING(codeException, "program returned error code", ret);

			ass.corrupted = false;
		}
		catch (const exception &e)
		{
			if (e.severity >= severityEnum::Error)
				throw;
			CAGE_LOG(severityEnum::Error, "database", string() + "processing asset '" + ass.name + "' failed");
		}
		catch (...)
		{
			CAGE_THROW_ERROR(exception, "unknown exception during asset processing");
		}
	}

	void load()
	{
		if (!pathExists(configPathDatabase))
			return;
		CAGE_LOG(severityEnum::Info, "database", string() + "loading database cache: '" + configPathDatabase + "'");
		holder<fileClass> f = newFile(configPathDatabase, fileMode(true, false));
		string b;
		if (!f->readLine(b) || b != databaseBegin)
			CAGE_THROW_ERROR(exception, "wrong file format");
		if (!f->readLine(b) || b != databaseVersion)
		{
			CAGE_LOG(severityEnum::Warning, "database", "assets database file version mismatch, database will not be loaded");
			return;
		}
		f->read(&timestamp, sizeof(timestamp));
		corruptedDatabanks.load(f.get());
		assets.load(f.get());
		if (!f->readLine(b) || b != databaseEnd)
			CAGE_THROW_ERROR(exception, "wrong file end");
		f->close();
		CAGE_LOG(severityEnum::Info, "database", string() + "loaded " + assets.size() + " asset entries");
	}

	void write(holder<fileClass> &f, const string &s)
	{
		f->write(s.c_str(), s.length());
		f->write(" ", 1);
	}

	void save()
	{
		// save database
		if (!((string)configPathDatabase).empty())
		{
			CAGE_LOG(severityEnum::Info, "database", string() + "saving database cache: '" + configPathDatabase + "'");
			holder<fileClass> f = newFile(configPathDatabase, fileMode(false, true));
			f->writeLine(databaseBegin);
			f->writeLine(databaseVersion);
			f->write(&timestamp, sizeof(timestamp));
			corruptedDatabanks.save(f.get());
			assets.save(f.get());
			f->writeLine(databaseEnd);
			f->close();
			CAGE_LOG(severityEnum::Info, "database", string() + "saved " + assets.size() + " asset entries");
		}

		// save reverse
		if (!((string)configPathReverse).empty())
		{
			holder<fileClass> f = newFile(configPathReverse, fileMode(false, true, true));
			std::vector <std::pair <string, const assetStruct*> > items;
			for (holderSet <assetStruct>::iterator ita = assets.begin(), eta = assets.end(); ita != eta; ita++)
			{
				const assetStruct &ass = **ita;
				items.push_back(std::pair <string, const assetStruct*>(ass.outputPath(), &ass));
			}
			struct cmp
			{
				const bool operator () (const std::pair <string, const assetStruct*> &a, const std::pair <string, const assetStruct*> &b) const
				{
					return a.first < b.first;
				}
			};
			std::sort(items.begin(), items.end(), cmp());
			f->writeLine("<hash>     <asset name>                                                 <scheme>        <databank>");
			for (std::vector <std::pair <string, const assetStruct*> >::iterator ita = items.begin(), eta = items.end(); ita != eta; ita++)
			{
				const assetStruct &ass = *ita->second;
				write(f, ita->first.fill(10));
				if (ass.corrupted)
					write(f, "CORRUPTED");
				write(f, ass.name.fill(60));
				write(f, ass.scheme.fill(15));
				write(f, ass.databank.fill(30));
				f->writeLine("");
			}
		}

		// save forward
		if (!((string)configPathForward).empty())
		{
			holder<fileClass> f = newFile(configPathForward, fileMode(false, true, true));
			std::vector <std::pair <string, const assetStruct*> > items;
			for (holderSet <assetStruct>::iterator ita = assets.begin(), eta = assets.end(); ita != eta; ita++)
			{
				const assetStruct &ass = **ita;
				items.push_back(std::pair <string, const assetStruct*>(ass.name, &ass));
			}
			struct cmp
			{
				const bool operator () (const std::pair <string, const assetStruct*> &a, const std::pair <string, const assetStruct*> &b) const
				{
					return a.first < b.first;
				}
			};
			std::sort(items.begin(), items.end(), cmp());
			f->writeLine("<hash>     <asset name>                                                 <scheme>        <databank>");
			for (std::vector <std::pair <string, const assetStruct*> >::iterator ita = items.begin(), eta = items.end(); ita != eta; ita++)
			{
				const assetStruct &ass = *ita->second;
				write(f, ass.outputPath().fill(10));
				if (ass.corrupted)
					write(f, "CORRUPTED");
				write(f, ass.name.fill(60));
				write(f, ass.scheme.fill(15));
				write(f, ass.databank.fill(30));
				f->writeLine("");
			}
		}
	}

	bool isNameDatabank(const string &name)
	{
		return name.isPattern("", "", ".asset");
	}

	bool isNameIgnored(const string &name)
	{
		for (stringSet::iterator it = configIgnoreExtensions.begin(), et = configIgnoreExtensions.end(); it != et; it++)
		{
			if (name.isPattern("", "", *it))
				return true;
		}
		for (stringSet::iterator it = configIgnorePaths.begin(), et = configIgnorePaths.end(); it != et; it++)
		{
			if (name.isPattern(*it, "", ""))
				return true;
		}
		return false;
	}

	typedef std::map<string, uint64, stringComparatorFast> filesMap;
	filesMap files;

	void findFiles(const string &path)
	{
		string pth = pathJoin(configPathInput, path);
		CAGE_LOG(severityEnum::Info, "database", string() + "checking path '" + pth + "'");
		holder<directoryListClass> d = newDirectoryList(pth);
		while (d->valid())
		{
			string p = pathJoin(path, d->name());
			if (d->isDirectory())
				findFiles(p);
			else if (!isNameIgnored(p))
			{
				try
				{
					detail::overrideBreakpoint o;
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

	holderSet<assetStruct>::iterator itg;
	holder<mutexClass> mut;
	holder<threadPoolClass> threads;

	void threadEntry(uint32, uint32)
	{
		while (true)
		{
			assetStruct *ass = nullptr;
			{
				scopeLock<mutexClass> m(mut);
				if (itg != assets.end())
					ass = const_cast<assetStruct*>(itg++->get());
			}
			if (!ass)
				break;
			if (ass->corrupted)
			{
				try
				{
					processAsset(*ass);
					if (!ass->corrupted)
					{
						scopeLock<mutexClass> m(mut);
						notifierNotify(ass->name);
					}
				}
				catch (const cage::exception &)
				{
					// do nothing
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "exception", "cought unknown exception in asset processing thread");
				}
			}
		}
	}

	static struct threadsInitializerClass
	{
	public:
		threadsInitializerClass()
		{
			mut = newMutex();
			threads = newThreadPool();
			threads->function.bind<&threadEntry>();
		}
	} threadsInitializerInstance;

	void checkAssets()
	{
		CAGE_LOG(severityEnum::Info, "database", "looking for assets to process");
		verdictValue = false;
		files.clear();
		findFiles("");
		holderSet<databankStruct> corruptedDbsCopy;
		std::swap(corruptedDbsCopy, corruptedDatabanks);
		corruptedDatabanks.clear();
		uint32 countBadDatabanks = 0;
		uint32 countCorrupted = 0;
		uint32 countMissingReferences = 0;

		for (holderSet<assetStruct>::iterator asIt = assets.begin(); asIt != assets.end();)
		{
			assetStruct &ass = *const_cast<assetStruct*>(asIt->get());

			// check for deleted, modified or corrupted databank
			if (files.find(ass.databank) == files.end() || corruptedDbsCopy.find(ass.databank) != corruptedDbsCopy.end() || files[ass.databank] > timestamp)
			{
				asIt = assets.erase(ass.name);
				continue;
			}

			// check for deleted or modified files
			for (stringSet::iterator fIt = ass.files.begin(), fEt = ass.files.end(); fIt != fEt; fIt++)
			{
				if (files.find(*fIt) == files.end() || files[*fIt] > timestamp)
					ass.corrupted = true;
			}

			asIt++;
		}

		// find new or modified databanks
		uint64 newestFile = 0;
		for (filesMap::iterator fIt = files.begin(), fEt = files.end(); fIt != fEt; fIt++)
		{
			newestFile = std::max(newestFile, fIt->second);
			if (isNameDatabank(fIt->first))
			{
				bool wasCorrupted = corruptedDbsCopy.find(fIt->first) != corruptedDbsCopy.end();
				if (fIt->second > timestamp || wasCorrupted)
				{
					bool corrupted = !parseDatabank(fIt->first);
					if (corrupted)
						corruptedDatabanks.insert(fIt->first);
					countBadDatabanks += corrupted;
				}
			}
		}

		stringSet outputHashes;
		for (holderSet<assetStruct>::iterator asIt = assets.begin(), asEt = assets.end(); asIt != asEt; asIt++)
		{
			assetStruct &ass = *const_cast<assetStruct*>(asIt->get());
			outputHashes.insert(ass.outputPath());
		}

		{ // reprocess assets
			itg = assets.begin();
			threads->run();
			CAGE_ASSERT_RUNTIME(itg == assets.end());
		}

		for (holderSet<assetStruct>::iterator asIt = assets.begin(), asEt = assets.end(); asIt != asEt; asIt++)
		{
			assetStruct &ass = *const_cast<assetStruct*>(asIt->get());

			// check internationalized name collisions
			if (!ass.internationalizedName.empty() && outputHashes.find(ass.internationalizedPath()) != outputHashes.end())
			{
				ass.corrupted = true;
				CAGE_LOG(severityEnum::Warning, "database", string() + "asset '" + ass.name + "' in databank '" + ass.databank + "' with internationalized name '" + ass.internationalizedName + "' collides with another asset");
			}

			// warn about missing references
			bool anyMissing = false;
			for (stringSet::iterator refIt = ass.references.begin(), refEt = ass.references.end(); refIt != refEt; refIt++)
			{
				if (!assets.exists(*refIt))
				{
					anyMissing = true;
					CAGE_LOG(severityEnum::Warning, "database", string() + "asset '" + ass.name + "' in databank '" + ass.databank + "' is missing reference '" + *refIt + "'");
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
		save();
		if (countBadDatabanks || countCorrupted || countMissingReferences)
		{
			CAGE_LOG(severityEnum::Warning, "verdict", string() +
				countBadDatabanks + " corrupted databanks, " +
				countCorrupted + " corrupted assets, " +
				countMissingReferences + " missing references");
		}
		else
		{
			CAGE_LOG(severityEnum::Info, "verdict", "ok");
			verdictValue = true;
		}
	}

	void loadSchemesDirectory(const string &dir)
	{
		string realpath = pathJoin(configPathScheme, dir);
		holder<directoryListClass> lst = newFilesystem()->directoryList(realpath);
		for (; lst->valid(); lst->next())
		{
			string name = pathJoin(dir, lst->name());
			if (lst->isDirectory())
			{
				loadSchemesDirectory(name);
				continue;
			}
			if (!name.isPattern("", "", ".scheme"))
				continue;
			schemeStruct s;
			s.name = name.subString(0, name.length() - 7);
			CAGE_LOG(severityEnum::Info, "database", string() + "loading scheme '" + s.name + "'");
			holder<iniClass> ini = newIni();
			ini->load(pathJoin(configPathScheme, name));
			s.parse(ini.get());
			schemes.insert(templates::move(s));
		}
	}
}

void start()
{
	// load schemes
	loadSchemesDirectory("");
	CAGE_LOG(severityEnum::Info, "database", string() + "Loaded " + schemes.size() + " schemes");

	// load
	timestamp = 0;
	if (configFromScratch)
		CAGE_LOG(severityEnum::Info, "database", "'from scratch' was defined. Previous cache is ignored.");
	else
		load();

	// find changes
	checkAssets();
}

void listen()
{
	CAGE_LOG(severityEnum::Info, "database", "initializing listening for changes");
	notifierInitialize(configNotifierPort);
	holder<changeWatcherClass> changes = newChangeWatcher();
	changes->registerPath(configPathInput);
	uint32 cycles = 0;
	bool changed = false;
	while (true)
	{
		notifierAccept();
		{
			string path = changes->waitForChange(1000 * 200);
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

