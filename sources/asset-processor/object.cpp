#include <vector>
#include <set>
#include <algorithm>

#include "processor.h"

#include <cage-core/configIni.h>
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

	holder<configIni> ini = newConfigIni();
	ini->load(inputFileName);

	string basePath = pathExtractPath(inputFile);
	std::vector<lodStruct> lods;
	std::set<uint32> deps;
	uint32 totalMeshes = 0;
	for (const string &section : ini->sections())
	{
		if (section == "render" || section == "skeletal_animation" || section == "texture_animation" || section == "size")
			continue;
		if (!section.isDigitsOnly())
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "section: '" + section + "'");
			CAGE_THROW_ERROR(exception, "invalid object definition: unknown section");
		}
		lodStruct ls;
		ls.index = section.toUint32();
		ls.threshold = real::Nan().value;
		for (const string &n : ini->items(section))
		{
			string v = ini->get(section, n);
			if (n.isDigitsOnly())
			{
				v = pathJoin(basePath, v);
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

	renderObjectHeader o;
	{
		detail::memset(&o, 0, sizeof(o));
		string c = ini->getString("render", "color");
		if (!c.empty())
			o.color = vec3::parse(c);
		else
			o.color = vec3::Nan();
		o.opacity = ini->getFloat("render", "opacity", real::Nan().value);
		string s = ini->getString("skeletal_animation", "name");
		if (!s.empty())
		{
			s = pathJoin(basePath, s);
			o.skelAnimName = hashString(s.c_str());
			deps.insert(o.skelAnimName);
			writeLine(string("ref=") + s);
		}
		o.skelAnimSpeed = ini->getFloat("skeletal_animation", "speed", real::Nan().value);
		o.skelAnimOffset = ini->getFloat("skeletal_animation", "offset", real::Nan().value);
		o.texAnimSpeed = ini->getFloat("texture_animation", "speed", real::Nan().value);
		o.texAnimOffset = ini->getFloat("texture_animation", "offset", real::Nan().value);
		o.worldSize = ini->getFloat("size", "world");
		o.pixelsSize = ini->getFloat("size", "pixels");
	}

	assetHeader h = initializeAssetHeaderStruct();
	h.dependenciesCount = numeric_cast<uint16>(deps.size());
	h.originalSize = sizeof(renderObjectHeader);
	h.originalSize += numeric_cast<uint32>(lods.size()) * sizeof(uint32);
	h.originalSize += numeric_cast<uint32>(lods.size() + 1) * sizeof(uint32);
	h.originalSize += totalMeshes * sizeof(uint32);

	holder<fileHandle> f = newFile(outputFileName, fileMode(false, true));
	f->write(&h, sizeof(h));
	for (uint32 it : deps)
		f->write(&it, sizeof(uint32));
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
