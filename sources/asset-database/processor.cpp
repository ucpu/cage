#include <cage-core/concurrent.h>
#include <cage-core/ini.h>
#include <cage-core/process.h>
#include <cage-core/threadPool.h>
#include <cage-core/debug.h>

#include "database.h"

#include <algorithm>
#include <vector>

#define CAGE_THROW_WARNING(EXCEPTION, ...) { EXCEPTION e(__FILE__, __LINE__, __FUNCTION__, ::cage::SeverityEnum::Warning, __VA_ARGS__); e.makeLog(); throw e; }

namespace
{
	const string databaseBegin = "cage-asset-database-begin";
	const string databaseVersion = "9";
	const string databaseEnd = "cage-asset-database-end";

	bool verdictValue = false;

	struct Databank
	{
		string name;

		explicit Databank(const string &s = "") : name(s)
		{};

		void load(File *f)
		{
			read(f, name);
		}

		void save(File *f) const
		{
			write(f, name);
		}

		bool operator < (const Databank &other) const
		{
			return StringComparatorFast()(name, other.name);
		}
	};

	uint64 timestamp;
	HolderSet<Scheme> schemes;
	HolderSet<Asset> assets;
	HolderSet<Databank> corruptedDatabanks;

	bool parseDatabank(const string &path)
	{
		Holder<Ini> ini = newIni();
		try
		{
			ini->importFile(pathJoin(configPathInput, path));
		}
		catch (cage::Exception &)
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "invalid ini file in databank '" + path + "'");
			return false;
		}

		// load all sections
		uint32 errors = 0;
		for (const string &section : ini->sections())
		{
			// load scheme
			string scheme = ini->getString(section, "scheme");
			if (scheme.empty())
			{
				CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "undefined scheme in databank '" + path + "' in section '" + section + "'");
				errors++;
				continue;
			}
			Scheme *sch = schemes.retrieve(scheme);
			if (!sch)
			{
				CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "invalid scheme '" + scheme + "' in databank '" + path + "' in section '" + section + "'");
				errors++;
				continue;
			}

			const auto items = ini->items(section);

			// find invalid properties
			bool propertiesOk = true;
			for (const string &prop : items)
			{
				if (isDigitsOnly(prop) || prop == "scheme")
					continue;
				if (!sch->schemeFields.exists(prop))
				{
					CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "unknown property '" + prop + "' (value '" + ini->getString(section, prop) + "') in databank '" + path + "' in section '" + section + "'");
					propertiesOk = false;
				}
			}
			if (!propertiesOk)
			{
				errors++;
				continue;
			}

			// find all assets
			for (const string &assItem : items)
			{
				if (!isDigitsOnly(assItem))
					continue; // not an asset

				Asset ass;
				ass.scheme = scheme;
				ass.databank = path;
				ass.name = ini->getString(section, assItem);
				ass.name = pathJoin(pathExtractPath(ass.databank), ass.name);
				bool ok = true;

				// check for duplicate asset name
				// (in case of multiple databanks in one folder)
				if (assets.exists(ass.name))
				{
					CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "duplicate asset name '" + ass.name + "' in databank '" + path + "' in section '" + section + "'");
					Asset &ass2 = *const_cast<Asset*>(assets.retrieve(ass.name));
					CAGE_LOG(SeverityEnum::Note, "database", stringizer() + "with asset in databank '" + ass2.databank + "'");
					ass2.corrupted = true;
					ok = false;
				}

				// check for hash collisions
				for (const auto &it : assets)
				{
					Asset &ass2 = *it;
					if (ass2.outputPath() == ass.outputPath())
					{
						CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset output path collision '" + ass.name + "' in databank '" + path + "' in section '" + section + "'");
						CAGE_LOG(SeverityEnum::Note, "database", stringizer() + "with '" + ass2.name + "' in databank '" + ass2.databank + "'");
						ass2.corrupted = true;
						ok = false;
					}
				}

				// load asset properties
				if (ok)
				{
					for (const string &prop : items)
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

				assets.insert(templates::move(ass));
			}
		}

		// check unused
		if (errors == 0)
		{
			string s, t, v;
			if (ini->anyUnused(s, t, v))
			{
				CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "unused property/asset '" + t + "' (value '" + v + "') in databank '" + path + "' in section '" + s + "'");
				errors++;
			}
		}

		return errors == 0;
	}

	void processAsset(Asset &ass)
	{
		detail::OverrideBreakpoint OverrideBreakpoint;
		CAGE_LOG(SeverityEnum::Info, "asset", ass.name);
		ass.corrupted = true;
		ass.needNotify = true;
		ass.files.clear();
		ass.references.clear();
		ass.aliasName = "";
		Scheme *scheme = schemes.retrieve(ass.scheme);
		CAGE_ASSERT(scheme);
		try
		{
			if (!scheme->applyOnAsset(ass))
				CAGE_THROW_WARNING(Exception, "asset has invalid configuration");

			Holder<Process> prg = newProcess(scheme->processor);
			prg->writeLine(stringizer() + "inputDirectory=" + pathToAbs(configPathInput)); // inputDirectory
			prg->writeLine(stringizer() + "inputName=" + ass.name); // inputName
			prg->writeLine(stringizer() + "outputDirectory=" + pathToAbs(string(configPathIntermediate).empty() ? configPathOutput : configPathIntermediate)); // outputDirectory
			prg->writeLine(stringizer() + "outputName=" + ass.outputPath()); // outputName
			prg->writeLine(stringizer() + "schemeIndex=" + scheme->schemeIndex); // schemeIndex
			for (const auto &it : ass.fields)
				prg->writeLine(stringizer() + it.first + "=" + it.second);
			prg->writeLine("cage-end");

			bool begin = false, end = false;
			while (true)
			{
				string line = prg->readLine();
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
					string param = trim(split(line, "="));
					line = trim(line);
					if (param == "use")
					{
						if (pathIsAbs(line))
						{
							CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + line + "'");
							CAGE_THROW_WARNING(Exception, "assets use path must be relative");
						}
						if (!pathIsFile(pathJoin(pathToAbs(configPathInput), line)))
						{
							CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + line + "'");
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
							CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "previous: '" + ass.aliasName + "'");
							CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "current: '" + line + "'");
							CAGE_THROW_WARNING(Exception, "assets alias name cannot be overridden");
						}
					}
					else
					{
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "parameter: " + param);
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "value: " + line);
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
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "processing asset '" + ass.name + "' failed");
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
		CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "loading database cache: '" + (string)configPathDatabase + "'");
		Holder<File> f = newFile(configPathDatabase, FileMode(true, false));
		string b;
		if (!f->readLine(b) || b != databaseBegin)
			CAGE_THROW_ERROR(Exception, "invalid file format");
		if (!f->readLine(b) || b != databaseVersion)
		{
			CAGE_LOG(SeverityEnum::Warning, "database", "assets database file version mismatch, database will not be loaded");
			return;
		}
		f->read(bufferView<char>(timestamp));
		corruptedDatabanks.load(f.get());
		assets.load(f.get());
		if (!f->readLine(b) || b != databaseEnd)
			CAGE_THROW_ERROR(Exception, "wrong file end");
		f->close();
		CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "loaded " + assets.size() + " asset entries");
	}

	void save()
	{
		// save database
		if (!((string)configPathDatabase).empty())
		{
			CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "saving database cache: '" + (string)configPathDatabase + "'");
			Holder<File> f = newFile(configPathDatabase, FileMode(false, true));
			f->writeLine(databaseBegin);
			f->writeLine(databaseVersion);
			f->write(bufferView(timestamp));
			corruptedDatabanks.save(f.get());
			assets.save(f.get());
			f->writeLine(databaseEnd);
			f->close();
			CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "saved " + assets.size() + " asset entries");
		}

		// save list by hash
		if (!((string)configPathByHash).empty())
		{
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(configPathByHash, fm);
			std::vector<std::pair<string, const Asset*>> items;
			for (const auto &it : assets)
			{
				const Asset &ass = *it;
				items.push_back(std::pair<string, const Asset*>(ass.outputPath(), &ass));
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			f->writeLine("<hash>     <asset name>                                                                                         <scheme>        <databank>");
			for (const auto &it : items)
			{
				const Asset &ass = *it.second;
				f->writeLine(stringizer() + fill(it.first, 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.databank, 31));
			}
		}

		// save list by names
		if (!((string)configPathByName).empty())
		{
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(configPathByName, fm);
			std::vector<std::pair<string, const Asset*>> items;
			for (const auto &it : assets)
			{
				const Asset &ass = *it;
				items.push_back(std::pair<string, const Asset*>(ass.name, &ass));
			}
			std::sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
				return a.first < b.first;
			});
			f->writeLine("<hash>     <asset name>                                                                                         <scheme>        <databank>");
			for (const auto &it : items)
			{
				const Asset &ass = *it.second;
				f->writeLine(stringizer() + fill(it.second->outputPath(), 11) + (ass.corrupted ? "CORRUPTED " : "") + fill(ass.name, 101) + fill(ass.scheme, 16) + fill(ass.databank, 31));
			}
		}
	}

	bool isNameDatabank(const string &name)
	{
		return isPattern(name, "", "", ".assets");
	}

	bool isNameIgnored(const string &name)
	{
		for (const string &it : configIgnoreExtensions)
		{
			if (isPattern(name, "", "", it))
				return true;
		}
		for (const string &it : configIgnorePaths)
		{
			if (isPattern(name, it, "", ""))
				return true;
		}
		return false;
	}

	typedef std::map<string, uint64, StringComparatorFast> FilesMap;
	FilesMap files;

	void findFiles(const string &path)
	{
		string pth = pathJoin(configPathInput, path);
		CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "checking path '" + pth + "'");
		Holder<DirectoryList> d = newDirectoryList(pth);
		while (d->valid())
		{
			string p = pathJoin(path, d->name());
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
			const string f = pathJoin(configPathIntermediate, listIn->name());
			const string t = pathJoin(configPathOutput, listIn->name());
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

	HolderSet<Asset>::Iterator itg;
	Holder<Mutex> mut;
	Holder<ThreadPool> threads;

	void threadEntry(uint32, uint32)
	{
		while (true)
		{
			Asset *ass = nullptr;
			{
				ScopeLock<Mutex> m(mut);
				if (itg != assets.end())
					ass = const_cast<Asset*>(itg++->get());
			}
			if (!ass)
				break;
			if (ass->corrupted)
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
		}
	}

	static struct ThreadsInitializer
	{
	public:
		ThreadsInitializer()
		{
			mut = newMutex();
			threads = newThreadPool();
			threads->function.bind<&threadEntry>();
		}
	} threadsInitializerInstance;

	void checkAssets()
	{
		CAGE_LOG(SeverityEnum::Info, "database", "looking for assets to process");
		if (!string(configPathIntermediate).empty())
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
						corruptedDatabanks.insert(Databank(f.first));
					countBadDatabanks += corrupted;
				}
			}
		}

		StringSet outputHashes;
		for (const auto &it : assets)
			outputHashes.insert(it->outputPath());

		{ // reprocess assets
			itg = assets.begin();
			threads->run();
			CAGE_ASSERT(itg == assets.end());
		}

		for (const auto &it : assets)
		{
			Asset &ass = *it;

			// check alias name collisions
			if (!ass.aliasName.empty() && outputHashes.find(ass.aliasPath()) != outputHashes.end())
			{
				ass.corrupted = true;
				CAGE_LOG(SeverityEnum::Warning, "database", stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' with alias name '" + ass.aliasName + "' collides with another asset");
			}

			// warn about missing references
			bool anyMissing = false;
			for (const string &it : ass.references)
			{
				if (!assets.exists(it))
				{
					anyMissing = true;
					CAGE_LOG(SeverityEnum::Warning, "database", stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' is missing reference '" + it + "'");
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
			CAGE_LOG(SeverityEnum::Warning, "verdict", stringizer() +
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

	void loadSchemesDirectory(const string &dir)
	{
		string realpath = pathJoin(configPathSchemes, dir);
		Holder<DirectoryList> lst = newDirectoryList(realpath);
		for (; lst->valid(); lst->next())
		{
			string name = pathJoin(dir, lst->name());
			if (lst->isDirectory())
			{
				loadSchemesDirectory(name);
				continue;
			}
			if (!isPattern(name, "", "", ".scheme"))
				continue;
			Scheme s;
			s.name = subString(name, 0, name.length() - 7);
			CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "loading scheme '" + s.name + "'");
			Holder<Ini> ini = newIni();
			ini->importFile(pathJoin(configPathSchemes, name));
			s.parse(ini.get());
			schemes.insert(templates::move(s));
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
	CAGE_LOG(SeverityEnum::Info, "database", stringizer() + "loaded " + schemes.size() + " schemes");

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
