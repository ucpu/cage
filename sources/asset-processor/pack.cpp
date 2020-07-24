#include <cage-core/ini.h>
#include <cage-core/hashString.h>

#include "processor.h"

#include <map>
#include <set>

void processPack()
{
	writeLine(string("use=") + inputFile);

	Holder<Ini> ini = newIni();
	ini->importFile(inputFileName);

	std::set<uint32> assets;
	for (const string &section : ini->sections())
	{
		for (const string &n : ini->items(section))
		{
			if (!n.isDigitsOnly())
				CAGE_THROW_ERROR(Exception, "invalid asset pack definition");
			string v = ini->get(section, n);
			v = pathJoin(pathExtractPath(inputName), v);
			assets.insert(HashString(v.c_str()));
			writeLine(string("ref=") + v);
		}
	}

	AssetHeader h = initializeAssetHeader();
	h.dependenciesCount = numeric_cast<uint16>(assets.size());

	Holder<File> f = newFile(outputFileName, FileMode(false, true));
	f->write(bufferView(h));
	for (uint32 it : assets)
		f->write(bufferView(it));
	f->close();
}
