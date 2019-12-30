#include <cage-core/core.h>
#include <cage-core/files.h>
#include <cage-core/hashString.h>

using namespace cage;

#include "utilities.h"
#include "asset.h"

Asset::Asset() : corrupted(true), needNotify(false)
{}

void Asset::load(File *f)
{
	read(f, name);
	read(f, aliasName);
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

void Asset::save(File *f) const
{
	write(f, name);
	write(f, aliasName);
	write(f, scheme);
	write(f, databank);
	write(f, numeric_cast<uint32>(fields.size()));
	for (auto it : fields)
	{
		write(f, it.first);
		write(f, it.second);
	}
	write(f, numeric_cast<uint32>(files.size()));
	for (const string &it : files)
		write(f, it);
	write(f, numeric_cast<uint32>(references.size()));
	for (const string &it : references)
		write(f, it);
	write(f, corrupted);
}

string Asset::outputPath() const
{
	return string(HashString(name.c_str()));
}

string Asset::aliasPath() const
{
	return string(HashString(aliasName.c_str()));
}
