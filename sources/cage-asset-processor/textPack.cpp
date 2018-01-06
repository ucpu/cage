#include <map>

#include "processor.h"

#include <cage-core/utility/ini.h>
#include <cage-core/utility/hashString.h>

void processTextpack()
{
	writeLine(string("use=") + inputFile);

	holder<iniClass> ini = newIni();
	ini->load(inputFileName);

	std::map<string, string> texts;
	for (uint32 sec = 0; sec < ini->sectionCount(); sec++)
	{
		string section = ini->section(sec);
		for (uint32 itm = 0; itm < ini->itemCount(section); itm++)
		{
			string n = ini->item(section, itm);
			string v = ini->get(section, n);
			if (!section.isDigitsOnly())
				n = section + "/" + n;
			texts[n] = v;
		}
	}
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "loaded " + texts.size() + " texts");

	assetHeaderStruct h = initializeAssetHeaderStruct();
	string intr = properties("internationalized");
	if (!intr.empty())
	{
		writeLine(string() + "internationalized = " + intr);
		h.internationalizedName = hashString(intr.c_str());
	}
	h.originalSize = sizeof(uint32) + numeric_cast<uint32>(texts.size()) * sizeof(uint32) * 2;
	for (auto it = texts.begin(), et = texts.end(); it != et; it++)
		h.originalSize += numeric_cast<uint32>(it->second.length());

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	uint32 count = numeric_cast<uint32>(texts.size());
	f->write(&count, sizeof(uint32));
	for (std::map<string, string>::iterator ass = texts.begin(), asse = texts.end(); ass != asse; ass++)
	{
		uint32 name = hashString(ass->first.c_str());
		f->write(&name, sizeof(uint32));
		uint32 len = ass->second.length();
		f->write(&len, sizeof(uint32));
		f->write(ass->second.c_str(), len);
	}

	if (configGetBool("cage-asset-processor.textpack.preview"))
	{
		string dbgName = pathJoin(configGetString("cage-asset-processor.textpack.path", "asset-preview"), pathMakeValid(inputName) + ".txt");
		holder<fileClass> f = newFile(dbgName, fileMode(false, true, true));
		for (std::map <string, string>::iterator ass = texts.begin(), asse = texts.end(); ass != asse; ass++)
			f->writeLine(string(hashString(ass->first.c_str())).fill(10) + " " + ass->first + " = " + ass->second);
	}
}