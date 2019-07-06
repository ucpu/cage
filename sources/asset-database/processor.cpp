#include <algorithm>
#include <vector>
#include <set>
#include <map>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/fileUtils.h>
#include <cage-core/concurrent.h>
#include <cage-core/configIni.h>
#include <cage-core/process.h>
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

		void load(fileHandle *f)
		{
			read(f, name);
		}

		void save(fileHandle *f) const
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
		holder<configIni> ini = newConfigIni();
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
		for (const string &section : ini->sections())
		{
			// load assets group properties
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
			for (const string &item : ini->items(section))
			{
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
				for (const auto &it : assets)
				{
					assetStruct &ass2 = *it;
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
				for (const string &item : ini->items(section))
				{
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
		ass.needNotify = true;
		ass.files.clear();
		ass.references.clear();
		ass.internationalizedName = "";
		schemeStruct *scheme = schemes.retrieve(ass.scheme);
		CAGE_ASSERT_RUNTIME(scheme, "asset has invalid scheme");
		try
		{
			if (!scheme->applyOnAsset(ass))
				CAGE_THROW_WARNING(exception, "asset has invalid configuration");

			holder<processHandle> prg = newProcess(scheme->processor);
			prg->writeLine(pathToAbs(configPathInput)); // inputDirectory
			prg->writeLine(ass.name); // inputName
			prg->writeLine(pathToAbs(string(configPathIntermediate).empty() ? configPathOutput : configPathIntermediate)); // outputDirectory
			prg->writeLine(ass.outputPath()); // outputName
			prg->writeLine(ass.databank); // assetPath
			prg->writeLine(pathJoin(configPathScheme, ass.scheme)); // schemePath
			prg->writeLine(string(scheme->schemeIndex)); // schemeIndex
			for (const auto &it : ass.fields)
				prg->writeLine(string() + it.first + "=" + it.second);
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
				CAGE_THROW_WARNING(codeException, "processHandle returned error code", ret);

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
		if (!pathIsFile(configPathDatabase))
			return;
		CAGE_LOG(severityEnum::Info, "database", string() + "loading database cache: '" + configPathDatabase + "'");
		holder<fileHandle> f = newFile(configPathDatabase, fileMode(true, false));
		string b;
		if (!f->readLine(b) || b != databaseBegin)
			CAGE_THROW_ERROR(exception, "invalid file format");
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

	void write(holder<fileHandle> &f, const string &s)
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
			holder<fileHandle> f = newFile(configPathDatabase, fileMode(false, true));
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
			fileMode fm(false, true);
			fm.textual = true;
			holder<fileHandle> f = newFile(configPathReverse, fm);
			std::vector <std::pair <string, const assetStruct*> > items;
			for (const auto &it : assets)
			{
				const assetStruct &ass = *it;
				items.push_back(std::pair<string, const assetStruct*>(ass.outputPath(), &ass));
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			f->writeLine("<hash>     <asset name>                                                 <scheme>        <databank>");
			for (const auto &it : items)
			{
				const assetStruct &ass = *it.second;
				write(f, it.first.fill(10));
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
			fileMode fm(false, true);
			fm.textual = true;
			holder<fileHandle> f = newFile(configPathForward, fm);
			std::vector<std::pair<string, const assetStruct*>> items;
			for (const auto &it : assets)
			{
				const assetStruct &ass = *it;
				items.push_back(std::pair <string, const assetStruct*>(ass.name, &ass));
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			f->writeLine("<hash>     <asset name>                                                 <scheme>        <databank>");
			for (const auto &it : items)
			{
				const assetStruct &ass = *it.second;
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
		for (const string &it : configIgnoreExtensions)
		{
			if (name.isPattern("", "", it))
				return true;
		}
		for (const string &it : configIgnorePaths)
		{
			if (name.isPattern(it, "", ""))
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
		holder<directoryList> d = newDirectoryList(pth);
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

	void moveIntermediateFiles()
	{
		holder<directoryList> listIn = newDirectoryList(configPathIntermediate);
		holder<directoryList> listOut = newDirectoryList(configPathOutput); // keep the archive open until all files are written (this significantly speeds up the moving process, but it causes the process to keep all the files in memory)
		uint64 movedSize = 0;
		while (listIn->valid())
		{
			if (movedSize > configArchiveWriteThreshold)
			{
				listOut.clear(); // close the archive
				listOut = newDirectoryList(configPathOutput); // reopen the archive
				movedSize = 0;
			}
			string f = pathJoin(configPathIntermediate, listIn->name());
			string t = pathJoin(configPathOutput, listIn->name());
			movedSize += newFile(f, fileMode(true, false))->size();
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
				CAGE_ASSERT_RUNTIME(!it->corrupted);
				notifierNotify(it->name);
				const_cast<assetStruct*>(it.get())->needNotify = false;
			}
		}
	}

	holderSet<assetStruct>::iterator itg;
	holder<syncMutex> mut;
	holder<threadPool> threads;

	void threadEntry(uint32, uint32)
	{
		while (true)
		{
			assetStruct *ass = nullptr;
			{
				scopeLock<syncMutex> m(mut);
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
				}
				catch (const cage::exception &)
				{
					// do nothing
				}
				catch (...)
				{
					CAGE_LOG(severityEnum::Error, "exception", "caught unknown exception in asset processing threadHandle");
				}
			}
		}
	}

	static struct threadsInitializerClass
	{
	public:
		threadsInitializerClass()
		{
			mut = newSyncMutex();
			threads = newThreadPool();
			threads->function.bind<&threadEntry>();
		}
	} threadsInitializerInstance;

	void checkAssets()
	{
		CAGE_LOG(severityEnum::Info, "database", "looking for assets to process");
		if (!string(configPathIntermediate).empty())
			pathRemove(configPathIntermediate);
		verdictValue = false;
		files.clear();
		findFiles("");
		holderSet<databankStruct> corruptedDbsCopy;
		std::swap(corruptedDbsCopy, corruptedDatabanks);
		corruptedDatabanks.clear();
		uint32 countBadDatabanks = 0;
		uint32 countCorrupted = 0;
		uint32 countMissingReferences = 0;

		for (auto asIt = assets.begin(); asIt != assets.end();)
		{
			assetStruct &ass = **asIt;

			// check for deleted, modified or corrupted databank
			if (files.find(ass.databank) == files.end() || corruptedDbsCopy.find(ass.databank) != corruptedDbsCopy.end() || files[ass.databank] > timestamp)
			{
				asIt = assets.erase(ass.name);
				continue;
			}

			// check for deleted or modified files
			for (const string &f : ass.files)
			{
				if (files.find(f) == files.end() || files[f] > timestamp)
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
				bool wasCorrupted = corruptedDbsCopy.find(f.first) != corruptedDbsCopy.end();
				if (f.second > timestamp || wasCorrupted)
				{
					bool corrupted = !parseDatabank(f.first);
					if (corrupted)
						corruptedDatabanks.insert(f.first);
					countBadDatabanks += corrupted;
				}
			}
		}

		stringSet outputHashes;
		for (const auto &it : assets)
			outputHashes.insert(it->outputPath());

		{ // reprocess assets
			itg = assets.begin();
			threads->run();
			CAGE_ASSERT_RUNTIME(itg == assets.end());
		}

		for (const auto &it : assets)
		{
			assetStruct &ass = *it;

			// check internationalized name collisions
			if (!ass.internationalizedName.empty() && outputHashes.find(ass.internationalizedPath()) != outputHashes.end())
			{
				ass.corrupted = true;
				CAGE_LOG(severityEnum::Warning, "database", string() + "asset '" + ass.name + "' in databank '" + ass.databank + "' with internationalized name '" + ass.internationalizedName + "' collides with another asset");
			}

			// warn about missing references
			bool anyMissing = false;
			for (const string &it : ass.references)
			{
				if (!assets.exists(it))
				{
					anyMissing = true;
					CAGE_LOG(severityEnum::Warning, "database", string() + "asset '" + ass.name + "' in databank '" + ass.databank + "' is missing reference '" + it + "'");
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
		if (!string(configPathIntermediate).empty())
			moveIntermediateFiles();
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
			sendAllNotifications();
			CAGE_LOG(severityEnum::Info, "verdict", "ok");
			verdictValue = true;
		}
	}

	void loadSchemesDirectory(const string &dir)
	{
		string realpath = pathJoin(configPathScheme, dir);
		holder<directoryList> lst = newFilesystem()->listDirectory(realpath);
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
			holder<configIni> ini = newConfigIni();
			ini->load(pathJoin(configPathScheme, name));
			s.parse(ini.get());
			schemes.insert(templates::move(s));
		}
	}

	void checkOutputDir()
	{
		pathTypeFlags t = pathType(configPathOutput);
		if (configOutputArchive && (t & pathTypeFlags::NotFound) == pathTypeFlags::NotFound)
			return pathCreateArchive(configPathOutput);
		if ((t & pathTypeFlags::Archive) == pathTypeFlags::Archive)
			return;
		// the output is not an archive, output to it directly
		configPathIntermediate = "";
	}
}

void start()
{
	// load schemes
	loadSchemesDirectory("");
	CAGE_LOG(severityEnum::Info, "database", string() + "loaded " + schemes.size() + " schemes");

	// load
	timestamp = 0;
	if (configFromScratch)
		CAGE_LOG(severityEnum::Info, "database", "'from scratch' was defined. Previous cache is ignored.");
	else
		load();

	// find changes
	checkOutputDir();
	checkAssets();
}

void listen()
{
	CAGE_LOG(severityEnum::Info, "database", "initializing listening for changes");
	notifierInitialize(configNotifierPort);
	holder<filesystemWatcher> changes = newFilesystemWatcher();
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

