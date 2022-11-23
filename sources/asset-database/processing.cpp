#include <cage-core/process.h>
#include <cage-core/tasks.h>
#include <cage-core/debug.h>
#include <cage-core/math.h>
#include <cage-core/hashString.h>

#include "database.h"

#include <algorithm>
#include <vector>

extern ConfigString configPathInput;
extern ConfigString configPathOutput;
extern ConfigString configPathIntermediate;
extern std::set<String, StringComparatorFast> configIgnoreExtensions;
extern std::set<String, StringComparatorFast> configIgnorePaths;
extern std::map<String, Holder<Scheme>, StringComparatorFast> schemes;
extern std::map<String, Holder<Asset>, StringComparatorFast> assets;
extern std::set<String, StringComparatorFast> corruptedDatabanks;
extern std::set<uint32> injectedNames;
extern uint64 lastModificationTime;
bool verdictValue = false;

void loadSchemes();
bool databankParse(const String &path);
void databanksLoad();
void databanksSave();
void checkOutputDir();
void moveIntermediateFiles();
bool isNameDatabank(const String &name);
void notifierSendNotifications();
std::map<String, uint64, StringComparatorFast> findFiles();

namespace
{
	void processAsset(Asset &ass)
	{
		CAGE_LOG(SeverityEnum::Info, "asset", ass.name);

		ass.corrupted = true;
		ass.aliasName = "";

		detail::OverrideBreakpoint overrideBreakpoint;
		try
		{
			ass.files.clear();
			ass.references.clear();

			const auto &scheme = schemes.at(ass.scheme);
			if (!scheme->applyOnAsset(ass))
				CAGE_THROW_ERROR(Exception, "asset has invalid configuration");

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
					CAGE_THROW_ERROR(Exception, "processing thrown an error");
				}
				else if (line == "cage-begin")
				{
					if (end || begin)
						CAGE_THROW_ERROR(Exception, "unexpected cage-begin");
					begin = true;
				}
				else if (line == "cage-end")
				{
					if (end || !begin)
						CAGE_THROW_ERROR(Exception, "unexpected cage-end");
					end = true;
					break;
				}
				else if (begin && !end)
				{
					const String param = trim(split(line, "="));
					line = trim(line);
					if (param == "use")
					{
						if (pathIsAbs(line))
						{
							CAGE_LOG_THROW(Stringizer() + "path: '" + line + "'");
							CAGE_THROW_ERROR(Exception, "assets use path must be relative");
						}
						if (!pathIsFile(pathJoin(pathToAbs(configPathInput), line)))
						{
							CAGE_LOG_THROW(Stringizer() + "path: '" + line + "'");
							CAGE_THROW_ERROR(Exception, "assets use path does not exist");
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
							CAGE_LOG_THROW(Stringizer() + "previous: '" + ass.aliasName + "', current: '" + line + "'");
							CAGE_THROW_ERROR(Exception, "assets alias name cannot be overridden");
						}
					}
					else
					{
						CAGE_LOG_THROW(Stringizer() + "parameter: '" + param + "', value: '" + line + "'");
						CAGE_THROW_ERROR(Exception, "unknown parameter name");
					}
				}
				else
					CAGE_LOG(SeverityEnum::Note, "processor", line);
			}

			const sint32 ret = prg->wait();
			if (ret != 0)
				CAGE_THROW_ERROR(SystemError, "process returned error code", ret);

			if (ass.files.empty())
				CAGE_THROW_ERROR(Exception, "asset reported no used files");

			ass.corrupted = false;
			ass.needNotify = true;
		}
		catch (const Exception &e)
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "processing asset '" + ass.name + "' failed");
		}
		catch (...)
		{
			detail::logCurrentCaughtException();
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "unknown exception while processing asset '" + ass.name + "'");
		}
	}

	void processAsset(Asset *const &ass)
	{
		processAsset(*ass);
	}

	void detectAssetsToProcess()
	{
		const auto files = findFiles();

		for (auto asIt = assets.begin(); asIt != assets.end();)
		{
			Asset &ass = *asIt->second;

			// check for deleted, modified or corrupted databank
			if (files.count(ass.databank) == 0 || files.at(ass.databank) > lastModificationTime || corruptedDatabanks.count(ass.databank) > 0)
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

		// find new or modified databanks
		const auto corruptedDbsCopy = std::move(corruptedDatabanks);
		CAGE_ASSERT(corruptedDatabanks.empty());
		for (const auto &f : files)
		{
			if (!isNameDatabank(f.first))
				continue;
			if (f.second <= lastModificationTime && corruptedDbsCopy.count(f.first) == 0)
				continue;
			if (!databankParse(f.first))
				corruptedDatabanks.insert(f.first);
		}

		{ // update modification time
			lastModificationTime = 0;
			for (const auto &f : files)
				lastModificationTime = max(lastModificationTime, f.second);
		}
	}

	void validateAssets()
	{
		// check for output hash collisions
		std::map<uint32, Asset *> outputHashes;
		for (const auto &it : assets)
		{
			Asset &ass = *it.second;
			const uint32 outputHash = ass.outputPath();
			if (outputHashes.count(outputHash) > 0)
			{
				auto &ass2 = *outputHashes[outputHash];
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' has name hash collision with asset '" + ass2.name + "' in databank '" + ass2.databank + "'");
				ass.corrupted = true;
				ass2.corrupted = true;
			}
			else
				outputHashes[outputHash] = +it.second;
		}

		// check for alias hash collisions
		for (const auto &it : assets)
		{
			Asset &ass = *it.second;
			if (ass.aliasName.empty())
				continue;
			const uint32 aliasHash = ass.aliasPath();
			if (outputHashes.count(aliasHash) > 0)
			{
				auto &ass2 = *outputHashes[aliasHash];
				CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' has ALIAS hash collision with asset '" + ass2.name + "' in databank '" + ass2.databank + "'");
				ass.corrupted = true;
				ass2.corrupted = true;
			}
		}

		// warn about missing references
		uint32 countMissingReferences = 0;
		for (const auto &it : assets)
		{
			Asset &ass = *it.second;
			bool anyMissing = false;
			for (const String &it : ass.references)
			{
				if (assets.count(it) == 0 && injectedNames.count(HashString(it)) == 0)
				{
					anyMissing = true;
					CAGE_LOG(SeverityEnum::Warning, "database", Stringizer() + "asset '" + ass.name + "' in databank '" + ass.databank + "' is missing reference '" + it + "'");
				}
			}
			if (anyMissing)
				countMissingReferences++;
		}

		// count corrupted
		uint32 countCorruptedAssets = 0;
		for (const auto &it : assets)
			if (it.second->corrupted)
				countCorruptedAssets++;

		// verdict
		if (!corruptedDatabanks.empty() || countCorruptedAssets || countMissingReferences)
		{
			CAGE_LOG(SeverityEnum::Warning, "verdict", Stringizer() +
				corruptedDatabanks.size() + " corrupted databanks, " +
				countCorruptedAssets + " corrupted assets, " +
				countMissingReferences + " missing references");
		}
		else
		{
			CAGE_LOG(SeverityEnum::Info, "verdict", "ok");
			verdictValue = true;
		}
	}
}

void checkAssets()
{
	verdictValue = false;

	CAGE_LOG(SeverityEnum::Info, "database", "looking for assets to process");

	if (!String(configPathIntermediate).empty())
		pathRemove(configPathIntermediate);

	detectAssetsToProcess();

	{ // reprocess assets
		std::vector<Asset *> asses;
		asses.reserve(assets.size());
		for (const auto &it : assets)
			if (it.second->corrupted)
				asses.push_back(+it.second);
		tasksRunBlocking<Asset *const>("processing", Delegate<void(Asset *const &)>().bind<&processAsset>(), asses);
	}

	validateAssets();

	if (!String(configPathIntermediate).empty())
		moveIntermediateFiles();

	databanksSave();

	if (verdictValue)
		notifierSendNotifications();
}

void start()
{
	loadSchemes();
	databanksLoad();
	checkOutputDir();
	checkAssets();
}
