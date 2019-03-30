#include <map>

#include "processor.h"
#include <cage-core/ini.h>
#include <cage-core/hashString.h>

void processInterpack()
{
	writeLine(string("use=") + inputFile);

	holder<iniClass> ini = newIni();
	ini->load(inputFileName);

	std::map<uint32, std::map<string, string>> assets;
	for (uint32 sec = 0; sec < ini->sectionCount(); sec++)
	{
		string section = ini->section(sec);
		uint32 sch = section.toUint32();
		for (uint32 itm = 0; itm < ini->itemCount(section); itm++)
		{
			string n = ini->item(section, itm);
			string v = ini->get(section, n);
			v = pathJoin(pathExtractPath(inputName), v);
			assets[sch][n] = v;
			writeLine(string("ref=") + v);
		}
	}

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	for (std::map<uint32, std::map<string, string>>::iterator sch = assets.begin(), sche = assets.end(); sch != sche; sch++)
	{
		uint32 s = sch->first;
		for (std::map<string, string>::iterator ass = sch->second.begin(), asse = sch->second.end(); ass != asse; ass++)
		{
			uint32 a = hashString(ass->first.c_str());
			uint32 b = hashString(ass->second.c_str());
			f->write(&s, sizeof(uint32));
			f->write(&a, sizeof(uint32));
			f->write(&b, sizeof(uint32));
		}
	}

	if (configGetBool("cage-asset-processor.interpack.preview"))
	{
		string dbgName = pathJoin(configGetString("cage-asset-processor.interpack.path", "asset-preview"), pathReplaceInvalidCharacters(inputName) + ".txt");
		fileMode fm(false, true);
		fm.textual = true;
		holder<fileClass> f = newFile(dbgName, fm);
		for (std::map<uint32, std::map<string, string>>::iterator sch = assets.begin(), sche = assets.end(); sch != sche; sch++)
		{
			uint32 s = sch->first;
			for (std::map<string, string>::iterator ass = sch->second.begin(), asse = sch->second.end(); ass != asse; ass++)
			{
				uint32 a = hashString(ass->first.c_str());
				uint32 b = hashString(ass->second.c_str());
				f->writeLine(string(s).fill(2) + " " + string(a).fill(10) + " " + string(b).fill(10) + " " + ass->first + " = " + ass->second);
			}
		}
	}
}
