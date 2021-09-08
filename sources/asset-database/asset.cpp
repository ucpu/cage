#include <cage-core/hashString.h>
#include <cage-core/containerSerialization.h>

#include "database.h"

Serializer &operator << (Serializer &ser, const Asset &s)
{
	ser << s.name << s.aliasName << s.scheme << s.databank;
	ser << s.fields << s.files << s.references;
	ser << s.corrupted;
	return ser;
}

Deserializer &operator >> (Deserializer &des, Asset &s)
{
	des >> s.name >> s.aliasName >> s.scheme >> s.databank;
	des >> s.fields >> s.files >> s.references;
	des >> s.corrupted;
	return des;
}

String Asset::outputPath() const
{
	return Stringizer() + HashString(name);
}

String Asset::aliasPath() const
{
	return Stringizer() + HashString(aliasName);
}
