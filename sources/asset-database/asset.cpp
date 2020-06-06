#include <cage-core/hashString.h>

#include "database.h"

void Asset::load(File *f)
{
	read(f, name);
	read(f, aliasName);
	read(f, scheme);
	read(f, databank);
	uint32 m = 0;
	f->read(bytesView(m));
	for (uint32 j = 0; j < m; j++)
	{
		string t1, t2;
		read(f, t1);
		read(f, t2);
		fields[t1] = t2;
	}
	f->read(bytesView(m));
	for (uint32 j = 0; j < m; j++)
	{
		string t;
		read(f, t);
		files.insert(t);
	}
	f->read(bytesView(m));
	for (uint32 j = 0; j < m; j++)
	{
		string t;
		read(f, t);
		references.insert(t);
	}
	f->read(bytesView(corrupted));
}

void Asset::save(File *f) const
{
	write(f, name);
	write(f, aliasName);
	write(f, scheme);
	write(f, databank);
	uint32 m = numeric_cast<uint32>(fields.size());
	f->write(bytesView(m));
	for (auto it : fields)
	{
		write(f, it.first);
		write(f, it.second);
	}
	m = numeric_cast<uint32>(files.size());
	f->write(bytesView(m));
	for (const string &it : files)
		write(f, it);
	m = numeric_cast<uint32>(references.size());
	f->write(bytesView(m));
	for (const string &it : references)
		write(f, it);
	f->write(bytesView(corrupted));
}

string Asset::outputPath() const
{
	return string(HashString(name.c_str()));
}

string Asset::aliasPath() const
{
	return string(HashString(aliasName.c_str()));
}
