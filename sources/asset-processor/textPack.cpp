#include "processor.h"

#include <cage-core/ini.h>
#include <cage-core/hashString.h>

#include <map>

void processTextpack()
{
	writeLine(string("use=") + inputFile);

	Holder<Ini> ini = newIni();
	ini->importFile(inputFileName);

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
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loaded " + texts.size() + " texts");

	AssetHeader h = initializeAssetHeader();
	h.originalSize = sizeof(uint32) + numeric_cast<uint32>(texts.size()) * sizeof(uint32) * 2;
	for (auto it : texts)
		h.originalSize += numeric_cast<uint32>(it.second.length());

	Holder<File> f = newFile(outputFileName, FileMode(false, true));
	f->write(bufferView(h));
	uint32 count = numeric_cast<uint32>(texts.size());
	f->write(bufferView(count));
	for (auto it : texts)
	{
		uint32 name = HashString(it.first.c_str());
		f->write(bufferView(name));
		uint32 len = it.second.length();
		f->write(bufferView(len));
		f->write({ it.second.c_str(), it.second.c_str() + len });
	}
	f->close();

	if (configGetBool("cage-asset-processor/textpack/preview"))
	{
		string dbgName = pathJoin(configGetString("cage-asset-processor/textpack/path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + ".txt");
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> f = newFile(dbgName, fm);
		for (auto it : texts)
			f->writeLine(string(HashString(it.first.c_str())).fill(10) + " " + it.first + " = " + it.second);
	}
}
