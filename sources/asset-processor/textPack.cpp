#include <map>

#include "processor.h"

#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/textPack.h>

void processTextpack()
{
	writeLine(String("use=") + inputFile);

	Holder<Ini> ini = newIni();
	ini->importFile(inputFileName);

	std::map<String, String> texts;
	for (const String &section : ini->sections())
	{
		for (String n : ini->items(section))
		{
			String v = ini->get(section, n);
			if (!isDigitsOnly(section))
				n = section + "/" + n;
			texts[n] = v;
		}
	}
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "loaded " + texts.size() + " texts");

	if (configGetBool("cage-asset-processor/textpack/preview"))
	{
		String dbgName = pathJoin(configGetString("cage-asset-processor/textpack/path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + ".txt");
		FileMode fm(false, true);
		fm.textual = true;
		Holder<File> f = newFile(dbgName, fm);
		for (const auto &it : texts)
			f->writeLine(fill(String(Stringizer() + HashString(it.first)), 10) + " " + it.first + " = " + it.second);
	}

	Holder<TextPack> pack = newTextPack();
	for (const auto &it : texts)
		pack->set(HashString(it.first), it.second);

	Holder<PointerRange<char>> buff = pack->exportBuffer();
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (before compression): " + buff.size());
	Holder<PointerRange<char>> comp = compress(buff);
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (after compression): " + comp.size());

	AssetHeader h = initializeAssetHeader();
	h.originalSize = buff.size();
	h.compressedSize = comp.size();
	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(comp);
	f->close();
}
