#include <map>
#include <set>

#include "processor.h"

#include <cage-core/utility/ini.h>
#include <cage-core/utility/hashString.h>

void processPack()
{
	writeLine(string("use=") + inputFile);

	holder<iniClass> ini = newIni();
	ini->load(inputFileName);

	std::set<uint32> assets;
	for (uint32 sec = 0; sec < ini->sectionCount(); sec++)
	{
		string section = ini->section(sec);
		for (uint32 itm = 0; itm < ini->itemCount(section); itm++)
		{
			string n = ini->item(section, itm);
			if (!n.isDigitsOnly())
				CAGE_THROW_ERROR(exception, "invalid asset pack definition");
			string v = ini->get(section, n);
			v = pathJoin(pathExtractPath(inputName), v);
			assets.insert(hashString(v.c_str()));
			writeLine(string("ref=") + v);
		}
	}

	assetHeaderStruct h = initializeAssetHeaderStruct();
	h.dependenciesCount = numeric_cast<uint16>(assets.size());

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	for (auto it = assets.begin(), et = assets.end(); it != et; it++)
		f->write(&*it, sizeof(uint32));
}