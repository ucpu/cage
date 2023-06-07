#include "processor.h"

#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <set>

void processPack()
{
	writeLine(String("use=") + inputFile);

	Holder<Ini> ini = newIni();
	ini->importFile(inputFileName);

	std::set<uint32> assets;
	for (const String &section : ini->sections())
	{
		for (const String &n : ini->items(section))
		{
			if (!isDigitsOnly(n))
				CAGE_THROW_ERROR(Exception, "invalid asset pack definition");
			String v = ini->get(section, n);
			v = convertAssetPath(v);
			assets.insert(HashString(v));
		}
	}

	AssetHeader h = initializeAssetHeader();
	h.dependenciesCount = numeric_cast<uint16>(assets.size());

	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	for (uint32 it : assets)
		f->write(bufferView(it));
	f->close();
}
