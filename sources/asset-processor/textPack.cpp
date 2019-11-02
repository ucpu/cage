#include <map>

#include "processor.h"

#include <cage-core/configIni.h>
#include <cage-core/hashString.h>

void processTextpack()
{
	writeLine(string("use=") + inputFile);

	holder<configIni> ini = newConfigIni();
	ini->load(inputFileName);

	std::map<string, string> texts;
	for (const string &section : ini->sections())
	{
		for (string n : ini->items(section))
		{
			string v = ini->get(section, n);
			if (!section.isDigitsOnly())
				n = section + "/" + n;
			texts[n] = v;
		}
	}
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "loaded " + texts.size() + " texts");

	assetHeader h = initializeAssetHeaderStruct();
	h.originalSize = sizeof(uint32) + numeric_cast<uint32>(texts.size()) * sizeof(uint32) * 2;
	for (auto it : texts)
		h.originalSize += numeric_cast<uint32>(it.second.length());

	holder<fileHandle> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	uint32 count = numeric_cast<uint32>(texts.size());
	f->write(&count, sizeof(uint32));
	for (auto it : texts)
	{
		uint32 name = hashString(it.first.c_str());
		f->write(&name, sizeof(uint32));
		uint32 len = it.second.length();
		f->write(&len, sizeof(uint32));
		f->write(it.second.c_str(), len);
	}
	f->close();

	if (configGetBool("cage-asset-processor.textpack.preview"))
	{
		string dbgName = pathJoin(configGetString("cage-asset-processor.textpack.path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + ".txt");
		fileMode fm(false, true);
		fm.textual = true;
		holder<fileHandle> f = newFile(dbgName, fm);
		for (auto it : texts)
			f->writeLine(string(hashString(it.first.c_str())).fill(10) + " " + it.first + " = " + it.second);
	}
}
