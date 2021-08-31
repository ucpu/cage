#include <cage-core/hashString.h>

#include "database.h"

void Asset::load(File *f)
{
	read(f, name);
	read(f, aliasName);
	read(f, scheme);
	read(f, databank);
	uint32 m = 0;
	f->read(bufferView<char>(m));
	for (uint32 j = 0; j < m; j++)
	{
		String t1, t2;
		read(f, t1);
		read(f, t2);
		fields[t1] = t2;
	}
	f->read(bufferView<char>(m));
	for (uint32 j = 0; j < m; j++)
	{
		String t;
		read(f, t);
		files.insert(t);
	}
	f->read(bufferView<char>(m));
	for (uint32 j = 0; j < m; j++)
	{
		String t;
		read(f, t);
		references.insert(t);
	}
	f->read(bufferView<char>(corrupted));
}

void Asset::save(File *f) const
{
	write(f, name);
	write(f, aliasName);
	write(f, scheme);
	write(f, databank);
	uint32 m = numeric_cast<uint32>(fields.size());
	f->write(bufferView(m));
	for (const auto &it : fields)
	{
		write(f, it.first);
		write(f, it.second);
	}
	m = numeric_cast<uint32>(files.size());
	f->write(bufferView(m));
	for (const String &it : files)
		write(f, it);
	m = numeric_cast<uint32>(references.size());
	f->write(bufferView(m));
	for (const String &it : references)
		write(f, it);
	f->write(bufferView(corrupted));
}

String Asset::outputPath() const
{
	return Stringizer() + HashString(name);
}

String Asset::aliasPath() const
{
	return Stringizer() + HashString(aliasName);
}
