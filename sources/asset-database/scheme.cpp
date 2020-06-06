#include <cage-core/ini.h>
#include <cage-core/macros.h>

#include "database.h"

void Scheme::parse(Ini *ini)
{
	processor = ini->getString("scheme", "processor");
	if (processor.empty())
	{
		CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "scheme: " + name);
		CAGE_THROW_ERROR(Exception, "empty scheme processor field");
	}
	schemeIndex = ini->getUint32("scheme", "index", m);
	if (schemeIndex == m)
	{
		CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "scheme: " + name);
		CAGE_THROW_ERROR(Exception, "empty scheme index field");
	}

	{
		SchemeField fld;
		fld.name = "alias";
		fld.display = "alias";
		fld.hint = "common name to reference the asset in multipacks";
		fld.type = "string";
		CAGE_ASSERT(fld.valid());
		schemeFields.insert(templates::move(fld));
	}

	for (const string &section : ini->sections())
	{
		if (section == "scheme")
			continue;
		SchemeField fld;
		fld.name = section;
#define GCHL_GENERATE(NAME) fld.NAME = ini->getString(section, CAGE_STRINGIZE(NAME));
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, display, hint, type, min, max, values))
#undef GCHL_GENERATE
		fld.defaul = ini->getString(section, "default");
		if (fld.valid())
			schemeFields.insert(templates::move(fld));
		else
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "scheme: " + name);
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "field: " + fld.name);
			CAGE_THROW_ERROR(Exception, "invalid scheme field data");
		}
	}

	{
		string s, t, v;
		if (ini->anyUnused(s, t, v))
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "scheme: '" + name + "'");
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "section: '" + s + "'");
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "item: '" + t + "'");
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "value: '" + v + "'");
			CAGE_THROW_ERROR(Exception, "unused scheme property");
		}
	}
}

void Scheme::load(File *f)
{
	read(f, name);
	read(f, processor);
	f->read(bytesView(schemeIndex));
	uint32 m = 0;
	f->read(bytesView(m));
	for (uint32 j = 0; j < m; j++)
	{
		SchemeField c;
		read(f, c.name);
		read(f, c.type);
		read(f, c.min);
		read(f, c.max);
		read(f, c.defaul);
		read(f, c.values);
		schemeFields.insert(templates::move(c));
	}
}

void Scheme::save(File *f)
{
	write(f, name);
	write(f, processor);
	f->write(bytesView(schemeIndex));
	uint32 m = schemeFields.size();
	f->write(bytesView(m));
	for (const auto &it : schemeFields)
	{
		const SchemeField &c = *it;
		write(f, c.name);
		write(f, c.type);
		write(f, c.min);
		write(f, c.max);
		write(f, c.defaul);
		write(f, c.values);
	}
}

bool Scheme::applyOnAsset(Asset &ass)
{
	bool ok = true;
	for (const auto &it : schemeFields)
		ok = it->applyToAssetField(ass.fields[it->name], ass.name) && ok;
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

bool SchemeField::valid() const
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
			if (!min.empty() && !min.isReal())
				return false;
			if (!max.empty() && !max.isReal())
				return false;
			if (!min.empty() && !max.empty() && min.toDouble() > max.toDouble())
				return false;
			if (!defaul.empty())
			{
				if (!defaul.isReal())
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
	catch (const Exception &)
	{
		return false;
	}
	return true;
}

bool SchemeField::applyToAssetField(string &val, const string &assetName) const
{
	if (val.empty())
	{
		if (defaul.empty() && type != "string")
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "' is a required property");
			return false;
		}
		val = defaul;
		return true;
	}
	if (type == "bool")
	{
		if (!val.isBool())
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not bool");
			return false;
		}
		val = string(val.toBool());
	}
	else if (type == "uint32" || type == "sint32")
	{
		bool allowSign = type == "sint32";
		if (!val.isInteger(allowSign))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not integer");
			return false;
		}
		if (!min.empty() && (allowSign ? (val.toSint32() < min.toSint32()) : (val.toUint32() < min.toUint32())))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && (allowSign ? (val.toSint32() > max.toSint32()) : (val.toUint32() > max.toUint32())))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "real")
	{
		if (!val.isReal())
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not real");
			return false;
		}
		if (!min.empty() && val.toDouble() < min.toDouble())
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && val.toDouble() > max.toDouble())
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "string")
	{
		if (!min.empty() && val.length() < min.toUint32())
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too short");
			return false;
		}
		if (!max.empty() && val.length() > max.toUint32())
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too long");
			return false;
		}
	}
	else if (type == "enum")
	{
		if (val.find(',') != m)
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' contains comma");
			return false;
		}
		if (!valuesContainsValue(values, val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not listed in values");
			return false;
		}
	}
	return true;
}
