#include <cage-core/files.h> // pathSimplify
#include <cage-core/config.h>
#include <cage-core/ini.h>

#include "database.h"

ConfigString configPathInput("cage-asset-database/path/input", "data");
ConfigString configPathOutput("cage-asset-database/path/output", "assets");
ConfigString configPathIntermediate("cage-asset-database/path/intermediate", "assets-tmp");
ConfigString configPathDatabase("cage-asset-database/path/database", "assets-database");
ConfigString configPathByHash("cage-asset-database/path/listByHash", "assets-by-hash.txt");
ConfigString configPathByName("cage-asset-database/path/listByName", "assets-by-name.txt");
ConfigString configPathSchemes("cage-asset-database/path/schemes", "schemes");
ConfigSint32 configNotifierPort("cage-asset-database/database/port", 65042);
ConfigUint64 configArchiveWriteThreshold("cage-asset-database/database/archiveWriteThreshold", 256 * 1024 * 1024);
ConfigBool configListening("cage-asset-database/database/listening", false);
ConfigBool configFromScratch("cage-asset-database/database/fromScratch", false);
ConfigBool configOutputArchive("cage-asset-database/database/outputArchive", false);
std::set<String, StringComparatorFast> configIgnoreExtensions;
std::set<String, StringComparatorFast> configIgnorePaths;

void configParseCmd(int argc, const char *args[])
{
	{
		Holder<Ini> ini = newIni();
		ini->parseCmd(argc, args);
		configFromScratch = ini->cmdBool('s', "scratch", configFromScratch);
		configListening = ini->cmdBool('l', "listen", configListening);
		ini->checkUnusedWithHelp();
	}

	configIgnoreExtensions.insert(".tmp");
	configIgnoreExtensions.insert(".blend1");
	configIgnoreExtensions.insert(".blend2");
	configIgnoreExtensions.insert(".blend3");
	configIgnoreExtensions.insert(".blend@");
	configIgnorePaths.insert(".svn");
	configIgnorePaths.insert(".git");
	configIgnorePaths.insert(".vs");
	Holder<ConfigList> list = newConfigList();
	while (list->valid())
	{
		const String n = list->name();
		if (isPattern(n, "cage-asset-database.ignoreExtensions.", "", ""))
			configIgnoreExtensions.insert(list->getString());
		else if (isPattern(n, "cage-asset-database.ignorePaths.", "", ""))
			configIgnorePaths.insert(pathSimplify(list->getString()));
		CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + n + ": " + list->getString());
		list->next();
	}

	// sanitize paths
	configPathInput = pathSimplify(configPathInput);
	configPathOutput = pathSimplify(configPathOutput);
	configPathDatabase = pathSimplify(configPathDatabase);
	configPathByHash = pathSimplify(configPathByHash);
	configPathByName = pathSimplify(configPathByName);
	configPathSchemes = pathSimplify(configPathSchemes);
}
