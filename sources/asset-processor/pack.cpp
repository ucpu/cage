#include <map>
#include <set>

#include "processor.h"

#include <cage-core/configIni.h>
#include <cage-core/hashString.h>

void processPack()
{
	writeLine(string("use=") + inputFile);

	holder<configIni> ini = newConfigIni();
	ini->load(inputFileName);

	std::set<uint32> assets;
	for (const string &section : ini->sections())
	{
		for (const string &n : ini->items(section))
		{
			if (!n.isDigitsOnly())
				CAGE_THROW_ERROR(exception, "invalid asset pack definition");
			string v = ini->get(section, n);
			v = pathJoin(pathExtractPath(inputName), v);
			assets.insert(hashString(v.c_str()));
			writeLine(string("ref=") + v);
		}
	}

	assetHeader h = initializeAssetHeaderStruct();
	h.dependenciesCount = numeric_cast<uint16>(assets.size());

	holder<fileHandle> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	for (uint32 it : assets)
		f->write(&it, sizeof(uint32));
}
