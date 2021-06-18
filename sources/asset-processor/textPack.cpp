#include <cage-core/ini.h>
#include <cage-core/hashString.h>
#include <cage-core/textPack.h>

#include "processor.h"

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
			if (!isDigitsOnly(section))
				n = section + "/" + n;
			texts[n] = v;
		}
	}
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loaded " + texts.size() + " texts");

	if (configGetBool("cage-asset-processor/textpack/preview"))
	{
		string dbgName = pathJoin(configGetString("cage-asset-processor/textpack/path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + ".txt");
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> f = newFile(dbgName, fm);
		for (const auto &it : texts)
			f->writeLine(fill(string(stringizer() + HashString(it.first)), 10) + " " + it.first + " = " + it.second);
	}

	Holder<TextPack> pack = newTextPack();
	for (const auto &it : texts)
		pack->set(HashString(it.first), it.second);

	Holder<PointerRange<char>> buff = pack->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
