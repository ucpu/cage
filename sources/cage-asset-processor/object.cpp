#include <map>
#include <set>

#include "processor.h"

#include <cage-core/utility/ini.h>
#include <cage-core/utility/hashString.h>

namespace
{
	struct lodStruct
	{
		float threshold;
		std::set<uint32> meshes;
	};
}

void processObject()
{
	writeLine(string("use=") + inputFile);

	holder<iniClass> ini = newIni();
	ini->load(inputFileName);

	std::map<uint32, lodStruct> lods;
	std::set<uint32> deps;
	uint32 totalMeshes = 0;
	for (uint32 sec = 0; sec < ini->sectionCount(); sec++)
	{
		string section = ini->section(sec);
		if (section == "size" || section == "ref")
			continue;
		else if (!section.isDigitsOnly())
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "section: '" + section + "'");
			CAGE_THROW_ERROR(exception, "invalid object definition: unknown section");
		}
		uint32 lod = section.toUint32();
		lods[lod].threshold = numeric_cast<float>(lod);
		for (uint32 itm = 0; itm < ini->itemCount(section); itm++)
		{
			string n = ini->item(section, itm);
			string v = ini->get(section, n);
			if (n.isDigitsOnly())
			{
				v = pathJoin(pathExtractPath(inputName), v);
				uint32 h = hashString(v.c_str());
				lods[lod].meshes.insert(h);
				deps.insert(h);
				writeLine(string("ref=") + v);
			}
			else if (n == "threshold")
				lods[lod].threshold = v.toFloat();
			else
			{
				CAGE_LOG(severityEnum::Note, "exception", string() + "section: '" + section + "', item: '" + n + "', value: '" + v + "'");
				CAGE_THROW_ERROR(exception, "invalid object definition: unknown item");
			}
		}
		totalMeshes += numeric_cast<uint32>(lods[lod].meshes.size());
	}

	objectHeaderStruct o;
	detail::memset(&o, 0, sizeof(o));

	{ // sizes
		o.worldSize = ini->getFloat("size", "world");
		o.pixelsSize = ini->getFloat("size", "pixels");
	}

	{ // refs
		string s;
		s = ini->getString("ref", "shadower");
		if (!s.empty())
		{
			s = pathJoin(pathExtractPath(inputName), s);
			o.shadower = hashString(s.c_str());
			deps.insert(o.shadower);
			writeLine(string("ref=") + s);
		}
		s = ini->getString("ref", "collider");
		if (!s.empty())
		{
			s = pathJoin(pathExtractPath(inputName), s);
			o.collider = hashString(s.c_str());
			deps.insert(o.collider);
			writeLine(string("ref=") + s);
		}
	}

	assetHeaderStruct h = initializeAssetHeaderStruct();
	h.dependenciesCount = numeric_cast<uint16>(deps.size());
	h.originalSize = sizeof(objectHeaderStruct) + numeric_cast<uint32>(lods.size()) * (sizeof(float) + sizeof(uint32)) + totalMeshes * sizeof(uint32);

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	for (auto it = deps.begin(), et = deps.end(); it != et; it++)
		f->write(&*it, sizeof(uint32));
	o.lodsCount = numeric_cast<uint32>(lods.size());
	f->write(&o, sizeof(o));
	for (std::map<uint32, lodStruct>::iterator ld = lods.begin(), lde = lods.end(); ld != lde; ld++)
	{
		float thr = ld->second.threshold;
		f->write(&thr, sizeof(float));
		uint32 cnt = numeric_cast<uint32>(ld->second.meshes.size());
		f->write(&cnt, sizeof(uint32));
		for (std::set<uint32>::iterator msh = ld->second.meshes.begin(), mshe = ld->second.meshes.end(); msh != mshe; msh++)
		{
			uint32 m = *msh;
			f->write(&m, sizeof(uint32));
		}
	}
}
