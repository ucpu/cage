#include <cage-core/ini.h>
#include <cage-core/macros.h>

#include "database.h"

void Scheme::parse(Ini *ini)
{
	processor = ini->getString("scheme", "processor");
	if (processor.empty())
	{
		CAGE_LOG_THROW(stringizer() + "scheme: " + name);
		CAGE_THROW_ERROR(Exception, "empty scheme processor field");
	}
	schemeIndex = ini->getUint32("scheme", "index", m);
	if (schemeIndex == m)
	{
		CAGE_LOG_THROW(stringizer() + "scheme: " + name);
		CAGE_THROW_ERROR(Exception, "empty scheme index field");
	}

	{
		SchemeField fld;
		fld.name = "alias";
		fld.display = "alias";
		fld.hint = "common name to reference the asset in multipacks";
		fld.type = "string";
		CAGE_ASSERT(fld.valid());
		schemeFields.insert(std::move(fld));
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
			schemeFields.insert(std::move(fld));
		else
		{
			CAGE_LOG_THROW(stringizer() + "scheme: " + name);
			CAGE_LOG_THROW(stringizer() + "field: " + fld.name);
			CAGE_THROW_ERROR(Exception, "invalid scheme field data");
		}
	}

	{
		string s, t, v;
		if (ini->anyUnused(s, t, v))
		{
			CAGE_LOG_THROW(stringizer() + "scheme: '" + name + "'");
			CAGE_LOG_THROW(stringizer() + "section: '" + s + "'");
			CAGE_LOG_THROW(stringizer() + "item: '" + t + "'");
			CAGE_LOG_THROW(stringizer() + "value: '" + v + "'");
			CAGE_THROW_ERROR(Exception, "unused scheme property");
		}
	}
}

void Scheme::load(File *f)
{
	read(f, name);
	read(f, processor);
	f->read(bufferView<char>(schemeIndex));
	uint32 m = 0;
	f->read(bufferView<char>(m));
	for (uint32 j = 0; j < m; j++)
	{
		SchemeField c;
		read(f, c.name);
		read(f, c.type);
		read(f, c.min);
		read(f, c.max);
		read(f, c.defaul);
		read(f, c.values);
		schemeFields.insert(std::move(c));
	}
}

void Scheme::save(File *f)
{
	write(f, name);
	write(f, processor);
	f->write(bufferView(schemeIndex));
	uint32 m = schemeFields.size();
	f->write(bufferView(m));
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
			if (trim(split(values, ",")) == value)
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
			if (!defaul.empty() && !isBool(defaul))
				return false;
		}
		else if (type == "uint32")
		{
			if (!values.empty())
				return false;
			if (!min.empty() && !isDigitsOnly(min))
				return false;
			if (!max.empty() && !isDigitsOnly(max))
				return false;
			if (!min.empty() && !max.empty() && toUint32(min) > toUint32(max))
				return false;
			if (!defaul.empty())
			{
				if (!isDigitsOnly(defaul))
					return false;
				if (!min.empty() && toUint32(defaul) < toUint32(min))
					return false;
				if (!max.empty() && toUint32(defaul) > toUint32(max))
					return false;
			}
		}
		else if (type == "sint32")
		{
			if (!values.empty())
				return false;
			if (!min.empty() && !isInteger(min))
				return false;
			if (!max.empty() && !isInteger(max))
				return false;
			if (!min.empty() && !max.empty() && toSint32(min) > toSint32(max))
				return false;
			if (!defaul.empty())
			{
				if (!isInteger(defaul))
					return false;
				if (!min.empty() && toSint32(defaul) < toSint32(min))
					return false;
				if (!max.empty() && toSint32(defaul) > toSint32(max))
					return false;
			}
		}
		else if (type == "real")
		{
			if (!values.empty())
				return false;
			if (!min.empty() && !isReal(min))
				return false;
			if (!max.empty() && !isReal(max))
				return false;
			if (!min.empty() && !max.empty() && toDouble(min) > toDouble(max))
				return false;
			if (!defaul.empty())
			{
				if (!isReal(defaul))
					return false;
				if (!min.empty() && toDouble(defaul) < toDouble(min))
					return false;
				if (!max.empty() && toDouble(defaul) > toDouble(max))
					return false;
			}
		}
		else if (type == "string")
		{
			if (!values.empty())
				return false;
			if (!min.empty() && !isDigitsOnly(min))
				return false;
			if (!max.empty() && !isDigitsOnly(max))
				return false;
			if (!min.empty() && !max.empty() && toUint32(min) > toUint32(max))
				return false;
			if (!defaul.empty())
			{
				if (!min.empty() && defaul.length() < toUint32(min))
					return false;
				if (!max.empty() && defaul.length() > toUint32(max))
					return false;
			}
		}
		else if (type == "enum")
		{
			if (values.empty() || !min.empty() || !max.empty())
				return false;
			if (valuesContainsValue(values, ""))
				return false;
			if (find(defaul, ',') != m)
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
		if (!isBool(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not bool");
			return false;
		}
		val = stringizer() + toBool(val);
	}
	else if (type == "uint32")
	{
		if (val.empty() || !isDigitsOnly(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not integer");
			return false;
		}
		if (!min.empty() && toUint32(val) < toUint32(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && toUint32(val) > toUint32(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "sint32")
	{
		if (!isInteger(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not integer");
			return false;
		}
		if (!min.empty() && toSint32(val) < toSint32(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && toSint32(val) > toSint32(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "real")
	{
		if (!isReal(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not real");
			return false;
		}
		if (!min.empty() && toDouble(val) < toDouble(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && toDouble(val) > toDouble(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "string")
	{
		if (!min.empty() && val.length() < toUint32(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too short");
			return false;
		}
		if (!max.empty() && val.length() > toUint32(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too long");
			return false;
		}
	}
	else if (type == "enum")
	{
		if (find(val, ',') != m)
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
