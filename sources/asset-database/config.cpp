#include <set>

#include <cage-core/core.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/configIni.h>

using namespace cage;

#include "config.h"

configString configPathInput("cage-asset-database/path/input", "data");
configString configPathOutput("cage-asset-database/path/output", "assets");
configString configPathIntermediate("cage-asset-database/path/intermediate", "assets-tmp");
configString configPathDatabase("cage-asset-database/path/database", "assets-database");
configString configPathByHash("cage-asset-database/path/listByHash", "assets-by-hash.txt");
configString configPathByName("cage-asset-database/path/listByName", "assets-by-name.txt");
configString configPathSchemes("cage-asset-database/path/schemes", "schemes");
configSint32 configNotifierPort("cage-asset-database/database/port", 65042);
configUint64 configArchiveWriteThreshold("cage-asset-database/database/archiveWriteThreshold", 256 * 1024 * 1024);
configBool configListening("cage-asset-database/database/listening", false);
configBool configFromScratch("cage-asset-database/database/fromScratch", false);
configBool configOutputArchive("cage-asset-database/database/outputArchive", false);
stringSet configIgnoreExtensions;
stringSet configIgnorePaths;

void configParseCmd(int argc, const char *args[])
{
	{
		holder<configIni> ini = newConfigIni(argc, args);
		configFromScratch = ini->cmdBool('s', "scratch", configFromScratch);
		configListening = ini->cmdBool('l', "listen", configListening);
		ini->checkUnused();
	}

	configIgnoreExtensions.insert(".tmp");
	configIgnoreExtensions.insert(".blend1");
	configIgnoreExtensions.insert(".blend2");
	configIgnoreExtensions.insert(".blend3");
	configIgnoreExtensions.insert(".blend@");
	configIgnorePaths.insert(".svn");
	configIgnorePaths.insert(".git");
	configIgnorePaths.insert(".vs");
	holder<configList> list = newConfigList();
	while (list->valid())
	{
		string n = list->name();
		if (n.isPattern("cage-asset-database.ignoreExtensions.", "", ""))
			configIgnoreExtensions.insert(list->getString());
		else if (n.isPattern("cage-asset-database.ignorePaths.", "", ""))
			configIgnorePaths.insert(pathSimplify(list->getString()));
		CAGE_LOG(severityEnum::Info, "config", stringizer() + n + ": " + list->getString());
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

