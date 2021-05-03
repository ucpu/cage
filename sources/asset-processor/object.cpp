#include <cage-core/ini.h>
#include <cage-core/hashString.h>

#include "processor.h"

#include <vector>
#include <set>
#include <algorithm>

namespace
{
	struct Lod
	{
		uint32 index;
		float threshold;
		std::set<uint32> models;
	};
}

void processObject()
{
	writeLine(string("use=") + inputFile);

	Holder<Ini> ini = newIni();
	ini->importFile(inputFileName);

	std::vector<Lod> lods;
	std::set<uint32> deps;
	uint32 totalModeles = 0;
	for (const string &section : ini->sections())
	{
		if (!isDigitsOnly(section))
			continue;
		Lod ls;
		ls.index = toUint32(section);
		ls.threshold = ini->getFloat(section, "threshold", real::Nan().value);
		for (const string &n : ini->items(section))
		{
			string v = ini->getString(section, n);
			if (!isDigitsOnly(n))
				continue;
			v = convertAssetPath(v);
			uint32 h = HashString(v);
			ls.models.insert(h);
			deps.insert(h);
		}
		totalModeles += numeric_cast<uint32>(ls.models.size());
		lods.push_back(std::move(ls));
	}

	for (Lod &ls : lods)
	{
		if (!real(ls.threshold).valid())
			ls.threshold = float(lods.size() - ls.index) / lods.size();
	}

	std::sort(lods.begin(), lods.end(), [](const Lod &a, const Lod &b) {
		return a.threshold > b.threshold;
	});

	RenderObjectHeader o;
	{
		detail::memset(&o, 0, sizeof(o));
		string c = ini->getString("render", "color");
		if (!c.empty())
			o.color = vec3::parse(c);
		else
			o.color = vec3::Nan();
		o.intensity = ini->getFloat("render", "intensity", real::Nan().value);
		o.opacity = ini->getFloat("render", "opacity", real::Nan().value);
		string s = ini->getString("skeletalAnimation", "name");
		if (!s.empty())
		{
			s = convertAssetPath(s);
			o.skelAnimName = HashString(s);
			deps.insert(o.skelAnimName);
		}
		o.skelAnimSpeed = ini->getFloat("skeletalAnimation", "speed", real::Nan().value);
		o.skelAnimOffset = ini->getFloat("skeletalAnimation", "offset", real::Nan().value);
		o.texAnimSpeed = ini->getFloat("textureAnimation", "speed", real::Nan().value);
		o.texAnimOffset = ini->getFloat("textureAnimation", "offset", real::Nan().value);
		o.worldSize = ini->getFloat("size", "world");
		o.pixelsSize = ini->getFloat("size", "pixels");
	}

	{
		string s, t, v;
		if (ini->anyUnused(s, t, v))
		{
			CAGE_LOG_THROW(stringizer() + "section: " + s);
			CAGE_LOG_THROW(stringizer() + "item: " + t);
			CAGE_LOG_THROW(stringizer() + "value: " + v);
			CAGE_THROW_ERROR(Exception, "unused value");
		}
	}

	AssetHeader h = initializeAssetHeader();
	h.dependenciesCount = numeric_cast<uint16>(deps.size());
	h.originalSize = sizeof(RenderObjectHeader);
	h.originalSize += numeric_cast<uint32>(lods.size()) * sizeof(uint32);
	h.originalSize += numeric_cast<uint32>(lods.size() + 1) * sizeof(uint32);
	h.originalSize += totalModeles * sizeof(uint32);

	MemoryBuffer buffer;
	Serializer ser(buffer);
	ser << h;
	for (uint32 it : deps)
		ser << it;
	o.lodsCount = numeric_cast<uint32>(lods.size());
	o.modelesCount = totalModeles;
	ser << o;
	for (const auto &ls : lods)
		ser << ls.threshold;
	uint32 accum = 0;
	for (const auto &ls : lods)
	{
		ser << accum;
		accum += numeric_cast<uint32>(ls.models.size());
	}
	ser << accum;
	for (const auto &ls : lods)
	{
		for (auto msh : ls.models)
			ser << msh;
	}
	Holder<File> f = writeFile(outputFileName);
	f->write(buffer);
	f->close();
}
