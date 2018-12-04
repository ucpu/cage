#include <vector>
#include <set>
#include <algorithm>

#include "processor.h"

#include <cage-core/ini.h>
#include <cage-core/hashString.h>

namespace
{
	struct lodStruct
	{
		uint32 index;
		float threshold;
		std::set<uint32> meshes;
	};
}

void processObject()
{
	writeLine(string("use=") + inputFile);

	holder<iniClass> ini = newIni();
	ini->load(inputFileName);

	std::vector<lodStruct> lods;
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
		lodStruct ls;
		ls.index = section.toUint32();
		ls.threshold = real::Nan.value;
		for (uint32 itm = 0; itm < ini->itemCount(section); itm++)
		{
			string n = ini->item(section, itm);
			string v = ini->get(section, n);
			if (n.isDigitsOnly())
			{
				v = pathJoin(pathExtractPath(inputName), v);
				uint32 h = hashString(v.c_str());
				ls.meshes.insert(h);
				deps.insert(h);
				writeLine(string("ref=") + v);
			}
			else if (n == "threshold")
				ls.threshold = v.toFloat();
			else
			{
				CAGE_LOG(severityEnum::Note, "exception", string() + "section: '" + section + "', item: '" + n + "', value: '" + v + "'");
				CAGE_THROW_ERROR(exception, "invalid object definition: unknown item");
			}
		}
		totalMeshes += numeric_cast<uint32>(ls.meshes.size());
		lods.push_back(templates::move(ls));
	}

	for (lodStruct &ls : lods)
	{
		if (!(ls.threshold == ls.threshold))
			ls.threshold = float(lods.size() - ls.index) / lods.size();
	}

	std::sort(lods.begin(), lods.end(), [](const lodStruct &a, const lodStruct &b) {
		return a.threshold > b.threshold;
	});

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
	h.originalSize = sizeof(objectHeaderStruct);
	h.originalSize += numeric_cast<uint32>(lods.size()) * sizeof(uint32);
	h.originalSize += numeric_cast<uint32>(lods.size() + 1) * sizeof(uint32);
	h.originalSize += totalMeshes * sizeof(uint32);

	holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	for (auto it = deps.begin(), et = deps.end(); it != et; it++)
		f->write(&*it, sizeof(uint32));
	o.lodsCount = numeric_cast<uint32>(lods.size());
	o.meshesCount = totalMeshes;
	f->write(&o, sizeof(o));
	for (const auto &ls : lods)
		f->write(&ls.threshold, sizeof(ls.threshold));
	uint32 accum = 0;
	for (const auto &ls : lods)
	{
		f->write(&accum, sizeof(accum));
		accum += numeric_cast<uint32>(ls.meshes.size());
	}
	f->write(&accum, sizeof(accum));
	for (const auto &ls : lods)
	{
		for (auto msh : ls.meshes)
			f->write(&msh, sizeof(msh));
	}
	f->close();
}
