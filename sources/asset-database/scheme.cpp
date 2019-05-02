#include <set>
#include <map>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/ini.h>

using namespace cage;

#include "utilities.h"
#include "holderSet.h"
#include "scheme.h"
#include "asset.h"

void schemeStruct::parse(iniClass *ini)
{
	processor = ini->getString("scheme", "processor");
	if (processor.empty())
	{
		CAGE_LOG(severityEnum::Note, "exception", string() + "scheme: " + name);
		CAGE_THROW_ERROR(exception, "empty scheme processor field");
	}
	schemeIndex = ini->getUint32("scheme", "index", m);
	if (schemeIndex == m)
	{
		CAGE_LOG(severityEnum::Note, "exception", string() + "scheme: " + name);
		CAGE_THROW_ERROR(exception, "empty scheme index field");
	}
	if (ini->itemsCount("scheme") != 2)
	{
		CAGE_LOG(severityEnum::Note, "exception", string() + "scheme: " + name);
		CAGE_THROW_ERROR(exception, "invalid fields in scheme section");
	}

	for (uint32 sectionIndex = 0, sectionIndexEnd = ini->sectionsCount(); sectionIndex != sectionIndexEnd; sectionIndex++)
	{
		string section = ini->section(sectionIndex);
		if (section == "scheme")
			continue;
		schemeFieldStruct fld;
		fld.name = section;
		fld.defaul = ini->getString(section, "default");
#define GCHL_GENERATE(NAME) fld.NAME = ini->getString(section, CAGE_STRINGIZE(NAME));
		GCHL_GENERATE(type);
		GCHL_GENERATE(min);
		GCHL_GENERATE(max);
		GCHL_GENERATE(values);
#undef GCHL_GENERATE
		if (fld.valid())
			schemeFields.insert(templates::move(fld));
		else
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "scheme: " + name);
			CAGE_LOG(severityEnum::Note, "exception", string() + "field: " + fld.name);
			CAGE_THROW_ERROR(exception, "invalid scheme field data");
		}
	}
}

void schemeStruct::load(fileClass *f)
{
	read(f, name);
	read(f, processor);
	read(f, schemeIndex);
	uint32 m = 0;
	read(f, m);
	for (uint32 j = 0; j < m; j++)
	{
		schemeFieldStruct c;
		read(f, c.name);
		read(f, c.type);
		read(f, c.min);
		read(f, c.max);
		read(f, c.defaul);
		read(f, c.values);
		schemeFields.insert(templates::move(c));
	}
}

void schemeStruct::save(fileClass *f)
{
	write(f, name);
	write(f, processor);
	write(f, schemeIndex);
	write(f, schemeFields.size());
	for (holderSet<schemeFieldStruct>::iterator i = schemeFields.begin(), e = schemeFields.end(); i != e; i++)
	{
		const schemeFieldStruct &c = **i;
		write(f, c.name);
		write(f, c.type);
		write(f, c.min);
		write(f, c.max);
		write(f, c.defaul);
		write(f, c.values);
	}
}

bool schemeStruct::applyOnAsset(assetStruct &ass)
{
	bool ok = true;
	for (holderSet <schemeFieldStruct>::iterator it = schemeFields.begin(), et = schemeFields.end(); it != et; it++)
		ok = (*it)->applyToAssetField(ass.fields[(*it)->name], ass.name) && ok;
	return ok;
}

namespace
{
	const bool valuesContainsValue(string values, const string &value)
	{
		while (!values.empty())
		{
			if (values.split(",").trim() == value)
				return true;
		}
		return false;
	}
}

bool schemeFieldStruct::valid() const
{
	try
	{
		if (name.empty())
			return false;
		if (type == "bool")
		{
			if (!values.empty() || !min.empty() || !max.empty())
				return false;
			if (!defaul.empty() && !defaul.isBool())
				return false;
		}
		else if (type == "uint32" || type == "sint32")
		{
			bool allowSign = type == "sint32";
			if (!values.empty())
				return false;
			if (!min.empty() && !min.isInteger(allowSign))
				return false;
			if (!max.empty() && !max.isInteger(allowSign))
				return false;
			if (!min.empty() && !max.empty() && (allowSign ? (min.toSint32() > max.toSint32()) : (min.toUint32() > max.toUint32())))
				return false;
			if (!defaul.empty())
			{
				if (!defaul.isInteger(allowSign))
					return false;
				if (!min.empty() && (allowSign ? (defaul.toSint32() < min.toSint32()) : (defaul.toUint32() < min.toUint32())))
					return false;
				if (!max.empty() && (allowSign ? (defaul.toSint32() > max.toSint32()) : (defaul.toUint32() > max.toUint32())))
					return false;
			}
		}
		else if (type == "real")
		{
			if (!values.empty())
				return false;
			if (!min.empty() && !min.isReal(true))
				return false;
			if (!max.empty() && !max.isReal(true))
				return false;
			if (!max.empty() && !max.isReal(true))
				return false;
			if (!min.empty() && !max.empty() && min.toDouble() > max.toDouble())
				return false;
			if (!defaul.empty())
			{
				if (!defaul.isReal(true))
					return false;
				if (!min.empty() && defaul.toDouble() < min.toDouble())
					return false;
				if (!max.empty() && defaul.toDouble() > max.toDouble())
					return false;
			}
		}
		else if (type == "string")
		{
			if (!values.empty())
				return false;
			if (!min.empty() && !min.isInteger(false))
				return false;
			if (!max.empty() && !max.isInteger(false))
				return false;
			if (!min.empty() && !max.empty() && min.toUint32() > max.toUint32())
				return false;
			if (!defaul.empty())
			{
				if (!min.empty() && defaul.length() < min.toUint32())
					return false;
				if (!max.empty() && defaul.length() > max.toUint32())
					return false;
			}
		}
		else if (type == "enum")
		{
			if (values.empty() || !min.empty() || !max.empty())
				return false;
			if (valuesContainsValue(values, ""))
				return false;
			if (defaul.find(',') != m)
				return false;
			if (!defaul.empty() && !valuesContainsValue(values, defaul))
				return false;
		}
		else
			return false;
	}
	catch (const exception &)
	{
		return false;
	}
	return true;
}

bool schemeFieldStruct::applyToAssetField(string &val, const string &assetName) const
{
	if (val.empty())
	{
		if (defaul.empty() && type != "string")
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "' is a required property");
			return false;
		}
		val = defaul;
		return true;
	}
	if (type == "bool")
	{
		if (!val.isBool())
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not bool");
			return false;
		}
		val = val.toBool();
	}
	else if (type == "uint32" || type == "sint32")
	{
		bool allowSign = type == "sint32";
		if (!val.isInteger(allowSign))
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not integer");
			return false;
		}
		if (!min.empty() && (allowSign ? (val.toSint32() < min.toSint32()) : (val.toUint32() < min.toUint32())))
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && (allowSign ? (val.toSint32() > max.toSint32()) : (val.toUint32() > max.toUint32())))
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "real")
	{
		if (!val.isReal(true))
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not real");
			return false;
		}
		if (!min.empty() && val.toDouble() < min.toDouble())
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && val.toDouble() > max.toDouble())
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "string")
	{
		if (!min.empty() && val.length() < min.toUint32())
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too short");
			return false;
		}
		if (!max.empty() && val.length() > max.toUint32())
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too long");
			return false;
		}
	}
	else if (type == "enum")
	{
		if (val.find(',') != m)
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' contains comma");
			return false;
		}
		if (!valuesContainsValue(values, val))
		{
			CAGE_LOG(severityEnum::Error, "database", string() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not listed in values");
			return false;
		}
	}
	return true;
}
