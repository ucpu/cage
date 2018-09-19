#include <set>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/config.h>
#include <cage-core/filesystem.h>
#include <cage-core/cmdOptions.h>

using namespace cage;

#include "config.h"

configString configPathInput("cage-asset-database.path.input", "data");
configString configPathOutput("cage-asset-database.path.output", "assets");
configString configPathDatabase("cage-asset-database.path.database", "asset-database.bin");
configString configPathReverse("cage-asset-database.path.reverse", "asset-reverse.txt");
configString configPathForward("cage-asset-database.path.forward", "asset-forward.txt");
configString configPathScheme("cage-asset-database.path.scheme", "schemes");
configSint32 configNotifierPort("cage-asset-database.database.port", 65042);
configBool configListening("cage-asset-database.database.listening", false);
configBool configFromScratch("cage-asset-database.database.fromscratch", false);
stringSet configIgnoreExtensions;
stringSet configIgnorePaths;

void configParseCmd(int argc, const char *args[])
{
	holder<cmdOptionsClass> cso = newCmdOptions(argc, args, "sl");
	while (cso->next())
	{
		switch (cso->option())
		{
		case 's':
			configFromScratch = true;
			break;
		case 'l':
			configListening = true;
			break;
		}
	}

	configIgnoreExtensions.insert(".tmp");
	configIgnoreExtensions.insert(".blend1");
	configIgnoreExtensions.insert(".blend2");
	configIgnoreExtensions.insert(".blend3");
	configIgnoreExtensions.insert(".blend@");
	configIgnorePaths.insert(".svn");
	configIgnorePaths.insert(".git");
	configIgnorePaths.insert(".vs");
	holder<configListClass> list = newConfigList();
	while (list->valid())
	{
		string n = list->name();
		if (n.pattern("cage-asset-database.ignoreExtensions.", "", ""))
			configIgnoreExtensions.insert(list->getString());
		else if (n.pattern("cage-asset-database.ignorePaths.", "", ""))
			configIgnorePaths.insert(pathSimplify(list->getString()));
		CAGE_LOG(severityEnum::Info, "config", string() + n + ": " + list->getString());
		list->next();
	}

	// sanitize paths
	configPathInput = pathSimplify(configPathInput);
	configPathOutput = pathSimplify(configPathOutput);
	configPathDatabase = pathSimplify(configPathDatabase);
	configPathReverse = pathSimplify(configPathReverse);
	configPathForward = pathSimplify(configPathForward);
	configPathScheme = pathSimplify(configPathScheme);
}

