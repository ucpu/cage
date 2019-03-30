#include <set>
#include <map>

#include <cage-core/core.h>
#include <cage-core/filesystem.h>
#include <cage-core/hashString.h>

using namespace cage;

#include "utilities.h"
#include "asset.h"

assetStruct::assetStruct() : corrupted(true), needNotify(false)
{}

void assetStruct::load(fileClass *f)
{
	read(f, name);
	read(f, internationalizedName);
	read(f, scheme);
	read(f, databank);
	uint32 m = 0;
	read(f, m);
	for (uint32 j = 0; j < m; j++)
	{
		string t1, t2;
		read(f, t1);
		read(f, t2);
		fields[t1] = t2;
	}
	read(f, m);
	for (uint32 j = 0; j < m; j++)
	{
		string t;
		read(f, t);
		files.insert(t);
	}
	read(f, m);
	for (uint32 j = 0; j < m; j++)
	{
		string t;
		read(f, t);
		references.insert(t);
	}
	read(f, corrupted);
}

void assetStruct::save(fileClass *f) const
{
	write(f, name);
	write(f, internationalizedName);
	write(f, scheme);
	write(f, databank);
	write(f, numeric_cast<uint32>(fields.size()));
	for (stringMap::const_iterator i = fields.begin(), e = fields.end(); i != e; i++)
	{
		write(f, i->first);
		write(f, i->second);
	}
	write(f, numeric_cast<uint32>(files.size()));
	for (stringSet::const_iterator i = files.begin(), e = files.end(); i != e; i++)
		write(f, *i);
	write(f, numeric_cast<uint32>(references.size()));
	for (stringSet::const_iterator i = references.begin(), e = references.end(); i != e; i++)
		write(f, *i);
	write(f, corrupted);
}

string assetStruct::outputPath() const
{
	return string(hashString(name.c_str()));
}

string assetStruct::internationalizedPath() const
{
	return string(hashString(internationalizedName.c_str()));
}
