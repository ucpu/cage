#include <cage-core/ini.h>
#include <cage-core/macros.h>
#include <cage-core/containerSerialization.h>
#include <cage-core/files.h>
#include <cage-core/config.h>

#include "database.h"

std::map<String, Holder<Scheme>> schemes;

void Scheme::parse(Ini *ini)
{
	processor = ini->getString("scheme", "processor");
	if (processor.empty())
	{
		CAGE_LOG_THROW(Stringizer() + "scheme: " + name);
		CAGE_THROW_ERROR(Exception, "empty scheme processor field");
	}
	schemeIndex = ini->getUint32("scheme", "index", m);
	if (schemeIndex == m)
	{
		CAGE_LOG_THROW(Stringizer() + "scheme: " + name);
		CAGE_THROW_ERROR(Exception, "empty scheme index field");
	}

	{
		SchemeField fld;
		fld.name = "alias";
		fld.display = "alias";
		fld.hint = "common name to reference the asset in multipacks";
		fld.type = "string";
		CAGE_ASSERT(fld.valid());
		schemeFields.emplace(fld.name, std::move(fld));
	}

	for (const String &section : ini->sections())
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
			schemeFields.emplace(fld.name, std::move(fld));
		else
		{
			CAGE_LOG_THROW(Stringizer() + "scheme: " + name);
			CAGE_LOG_THROW(Stringizer() + "field: " + fld.name);
			CAGE_THROW_ERROR(Exception, "invalid scheme field data");
		}
	}

	{
		String s, t, v;
		if (ini->anyUnused(s, t, v))
		{
			CAGE_LOG_THROW(Stringizer() + "scheme: '" + name + "'");
			CAGE_LOG_THROW(Stringizer() + "section: '" + s + "'");
			CAGE_LOG_THROW(Stringizer() + "item: '" + t + "'");
			CAGE_LOG_THROW(Stringizer() + "value: '" + v + "'");
			CAGE_THROW_ERROR(Exception, "unused scheme property");
		}
	}
}

Serializer &operator << (Serializer &ser, const SchemeField &s)
{
	ser << s.name;
	ser << s.type;
	ser << s.min;
	ser << s.max;
	ser << s.defaul;
	ser << s.values;
	return ser;
}

Deserializer &operator >> (Deserializer &des, SchemeField &s)
{
	des >> s.name;
	des >> s.type;
	des >> s.min;
	des >> s.max;
	des >> s.defaul;
	des >> s.values;
	return des;
}

Serializer &operator << (Serializer &ser, const Scheme &s)
{
	ser << s.name << s.processor << s.schemeIndex;
	ser << s.schemeFields;
	return ser;
}

Deserializer &operator >> (Deserializer &des, Scheme &s)
{
	des >> s.name >> s.processor >> s.schemeIndex;
	des >> s.schemeFields;
	return des;
}

bool Scheme::applyOnAsset(Asset &ass)
{
	bool ok = true;
	for (const auto &it : schemeFields)
		ok = it.second.applyToAssetField(ass.fields[it.first], ass.name) && ok;
	return ok;
}

namespace
{
	bool valuesContainsValue(const String &values_, const String &value)
	{
		String values = values_;
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

bool SchemeField::applyToAssetField(String &val, const String &assetName) const
{
	if (val.empty())
	{
		if (defaul.empty() && type != "string")
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "' is a required property");
			return false;
		}
		val = defaul;
		return true;
	}
	if (type == "bool")
	{
		if (!isBool(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not bool");
			return false;
		}
		val = Stringizer() + toBool(val);
	}
	else if (type == "uint32")
	{
		if (val.empty() || !isDigitsOnly(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not integer");
			return false;
		}
		if (!min.empty() && toUint32(val) < toUint32(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && toUint32(val) > toUint32(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "sint32")
	{
		if (!isInteger(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not integer");
			return false;
		}
		if (!min.empty() && toSint32(val) < toSint32(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && toSint32(val) > toSint32(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "real")
	{
		if (!isReal(val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not real");
			return false;
		}
		if (!min.empty() && toDouble(val) < toDouble(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too small");
			return false;
		}
		if (!max.empty() && toDouble(val) > toDouble(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too large");
			return false;
		}
	}
	else if (type == "string")
	{
		if (!min.empty() && val.length() < toUint32(min))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too short");
			return false;
		}
		if (!max.empty() && val.length() > toUint32(max))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is too long");
			return false;
		}
	}
	else if (type == "enum")
	{
		if (find(val, ',') != m)
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' contains comma");
			return false;
		}
		if (!valuesContainsValue(values, val))
		{
			CAGE_LOG(SeverityEnum::Error, "database", Stringizer() + "asset '" + assetName + "', field '" + this->name + "', value '" + val + "' is not listed in values");
			return false;
		}
	}
	return true;
}

namespace
{
	void loadSchemes(const String &dir)
	{
		const String realpath = pathJoin(configPathSchemes, dir);
		Holder<DirectoryList> lst = newDirectoryList(realpath);
		for (; lst->valid(); lst->next())
		{
			const String name = pathJoin(dir, lst->name());
			if (lst->isDirectory())
			{
				loadSchemes(name);
				continue;
			}
			if (!isPattern(name, "", "", ".scheme"))
				continue;
			Holder<Scheme> s = systemMemory().createHolder<Scheme>();
			s->name = subString(name, 0, name.length() - 7);
			CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loading scheme '" + s->name + "'");
			Holder<Ini> ini = newIni();
			ini->importFile(pathJoin(configPathSchemes, name));
			s->parse(+ini);
			schemes.emplace(s->name, std::move(s));
		}
	}
}

void loadSchemes()
{
	schemes.clear();

	loadSchemes("");

	CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "loaded " + schemes.size() + " schemes");
}
