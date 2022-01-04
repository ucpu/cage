#include <cage-core/hashString.h>
#include <cage-core/containerSerialization.h>

#include "database.h"

std::map<String, Holder<Asset>, StringComparatorFast> assets;

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

uint32 Asset::outputPath() const
{
	return HashString(name);
}

uint32 Asset::aliasPath() const
{
	return HashString(aliasName);
}
