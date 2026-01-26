#include <cage-core/assetsDatabase.h>
#include <cage-core/config.h>
#include <cage-core/files.h> // pathSimplify
#include <cage-core/ini.h>
#include <cage-core/logger.h>

using namespace cage;

ConfigString configPathDatabase("cage-asset-database/path/database", AssetsDatabaseCreateConfig().databasePath);
ConfigString configPathInput("cage-asset-database/path/input", AssetsDatabaseCreateConfig().inputPath);
ConfigString configPathIntermediate("cage-asset-database/path/intermediate", AssetsDatabaseCreateConfig().intermediatePath);
ConfigString configPathByNames("cage-asset-database/path/listByNames", AssetsDatabaseCreateConfig().outputListByNames);
ConfigString configPathByIds("cage-asset-database/path/listByIds", AssetsDatabaseCreateConfig().outputListByIds);
ConfigString configPathOutput("cage-asset-database/path/output", AssetsDatabaseCreateConfig().outputPath);
ConfigBool configOutputArchive("cage-asset-database/database/outputArchive", AssetsDatabaseCreateConfig().outputArchive);
ConfigString configPathSchemes("cage-asset-database/path/schemes", "schemes");
ConfigString configPathInjectedNames("cage-asset-database/path/injectNames", "");
ConfigSint32 configNotifierPort("cage-asset-database/database/port", 65042);
ConfigBool configFromScratch("cage-asset-database/database/fromScratch", false);
ConfigBool configListening("cage-asset-database/database/listening", false);

Holder<AssetsDatabase> database;

void listen();

bool logFilter(const cage::detail::LoggerInfo &info)
{
	return info.severity >= SeverityEnum::Error || String(info.component) == "exception" || String(info.component) == "asset" || String(info.component) == "database" || String(info.component) == "help";
}

int main(int argc, const char *args[])
{
	initializeConsoleLogger()->filter.bind<logFilter>();
	try
	{
		{
			Holder<Ini> ini = newIni();
			ini->parseCmd(argc, args);
			configFromScratch = ini->cmdBool('s', "scratch", configFromScratch);
			configListening = ini->cmdBool('l', "listen", configListening);
			ini->checkUnusedWithHelp();
		}

		if (configFromScratch)
		{
			CAGE_LOG(SeverityEnum::Info, "database", "starting from scratch");
			pathRemove(configPathDatabase);
		}

		{
			CAGE_LOG(SeverityEnum::Info, "database", "create database");
			AssetsDatabaseCreateConfig cfg;
			cfg.databasePath = configPathDatabase;
			cfg.inputPath = configPathInput;
			cfg.intermediatePath = configPathIntermediate;
			cfg.outputListByNames = configPathByNames;
			cfg.outputListByIds = configPathByIds;
			cfg.outputPath = configPathOutput;
			cfg.outputArchive = configOutputArchive;
			try
			{
				database = newAssetsDatabase(cfg);
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Info, "database", "failed loading database, starting from scratch");
				pathRemove(cfg.databasePath);
				database = newAssetsDatabase(cfg);
			}
			CAGE_LOG(SeverityEnum::Info, "database", "loading schemes");
			databaseLoadSchemes(+database, configPathSchemes);
		}

		if (!String(configPathInjectedNames).empty())
		{
			CAGE_LOG(SeverityEnum::Info, "database", "loading injected names");
			databaseInjectExternal(+database, configPathInjectedNames);
		}

		CAGE_LOG(SeverityEnum::Info, "database", "converting assets");
		try
		{
			database->convertAssets();
		}
		catch (...)
		{
			database->printIssues();
		}

		if (configListening)
		{
			CAGE_LOG(SeverityEnum::Info, "database", "waiting for changes");
			try
			{
				listen();
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Info, "database", "stopped");
			}
		}

		CAGE_LOG(SeverityEnum::Info, "database", database->status().print());
		return database->status().ok ? 0 : -1;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
