#include <set>

#include "processor.h"

#include <cage-core/hashString.h>
#include <cage-core/ini.h>

void processPack()
{
	processor->writeLine(String("use=") + processor->inputFile);

	Holder<Ini> ini = newIni();
	ini->importFile(processor->inputFileName);

	std::set<uint32> assets;
	for (const String &section : ini->sections())
	{
		for (const String &n : ini->items(section))
		{
			if (!isDigitsOnly(n))
				CAGE_THROW_ERROR(Exception, "invalid asset pack definition");
			String v = ini->get(section, n);
			v = processor->convertAssetPath(v);
			assets.insert(HashString(v));
		}
	}

	AssetHeader h = processor->initializeAssetHeader();
	h.dependenciesCount = numeric_cast<uint16>(assets.size());

	Holder<File> f = writeFile(processor->outputFileName);
	f->write(bufferView(h));
	for (uint32 it : assets)
		f->write(bufferView(it));
	f->close();
}
