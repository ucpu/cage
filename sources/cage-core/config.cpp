#include <map>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/ini.h>
#include <cage-core/config.h>

namespace cage
{
	namespace
	{
		mutexClass *mut()
		{
			static holder<mutexClass> *m = new holder<mutexClass>(newMutex()); // this leak is intentionall
			return m->get();
		}

		struct variable
		{
			configTypeEnum type;
			union
			{
				bool b;
				sint32 s32;
				uint32 u32;
				sint64 s64;
				uint64 u64;
				float f;
				double d;
			};
			holder<string> s;

			variable() : type(configTypeEnum::Undefined)
			{}

			~variable()
			{}

			void clear()
			{
				u64 = 0;
				type = configTypeEnum::Undefined;
			}

			void set(bool value) { type = configTypeEnum::Bool; b = value; }
			void set(sint32 value) { type = configTypeEnum::Sint32; s32 = value; }
			void set(uint32 value) { type = configTypeEnum::Uint32; u32 = value; }
			void set(sint64 value) { type = configTypeEnum::Sint64; s64 = value; }
			void set(uint64 value) { type = configTypeEnum::Uint64; u64 = value; }
			void set(float value) { type = configTypeEnum::Float; f = value; }
			void set(double value) { type = configTypeEnum::Double; d = value; }
			void set(const string &value) { if (!s) s = detail::systemArena().createHolder<string>(value); else *s = value; type = configTypeEnum::String; }
		};

		typedef std::map<string, holder<variable>, stringComparatorFast> varsType;

		bool varsConfigLoader()
		{
			if (pathExists("cage.ini"))
			{
				configLoadIni("cage.ini", "");
				return true;
			}
			return false;
		}

		varsType &vars()
		{
			static varsType *v = new varsType(); // intentionally left to leak
			static bool c = varsConfigLoader();
			return *v;
		}

		variable *getVar(const string &name)
		{
			CAGE_ASSERT_RUNTIME(!name.empty(), "variable name cannot be empty");
			variable *v = vars()[name].get();
			if (!v)
			{
				vars()[name] = detail::systemArena().createHolder<variable>();
				v = vars()[name].get();
			}
			return v;
		}

		template<class C> const C cast(const variable *v) {}
		template<> const bool cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return v->s32 != 0;
			case configTypeEnum::Uint32: return v->u32 != 0;
			case configTypeEnum::Sint64: return v->s64 != 0;
			case configTypeEnum::Uint64: return v->u64 != 0;
			case configTypeEnum::Float: return real(v->f) != real(0);
			case configTypeEnum::Double: return real(v->d) != real(0);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toBool();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const sint32 cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return numeric_cast<sint32>(v->s32);
			case configTypeEnum::Uint32: return numeric_cast<sint32>(v->u32);
			case configTypeEnum::Sint64: return numeric_cast<sint32>(v->s64);
			case configTypeEnum::Uint64: return numeric_cast<sint32>(v->u64);
			case configTypeEnum::Float:  return numeric_cast<sint32>(v->f);
			case configTypeEnum::Double: return numeric_cast<sint32>(v->d);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toSint32();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const uint32 cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return numeric_cast<uint32>(v->s32);
			case configTypeEnum::Uint32: return numeric_cast<uint32>(v->u32);
			case configTypeEnum::Sint64: return numeric_cast<uint32>(v->s64);
			case configTypeEnum::Uint64: return numeric_cast<uint32>(v->u64);
			case configTypeEnum::Float:  return numeric_cast<uint32>(v->f);
			case configTypeEnum::Double: return numeric_cast<uint32>(v->d);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toUint32();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const sint64 cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return numeric_cast<sint64>(v->s32);
			case configTypeEnum::Uint32: return numeric_cast<sint64>(v->u32);
			case configTypeEnum::Sint64: return numeric_cast<sint64>(v->s64);
			case configTypeEnum::Uint64: return numeric_cast<sint64>(v->u64);
			case configTypeEnum::Float:  return numeric_cast<sint64>(v->f);
			case configTypeEnum::Double: return numeric_cast<sint64>(v->d);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toSint64();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const uint64 cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return numeric_cast<uint64>(v->s32);
			case configTypeEnum::Uint32: return numeric_cast<uint64>(v->u32);
			case configTypeEnum::Sint64: return numeric_cast<uint64>(v->s64);
			case configTypeEnum::Uint64: return numeric_cast<uint64>(v->u64);
			case configTypeEnum::Float:  return numeric_cast<uint64>(v->f);
			case configTypeEnum::Double: return numeric_cast<uint64>(v->d);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toUint64();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const float cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return numeric_cast<float>(v->s32);
			case configTypeEnum::Uint32: return numeric_cast<float>(v->u32);
			case configTypeEnum::Sint64: return numeric_cast<float>(v->s64);
			case configTypeEnum::Uint64: return numeric_cast<float>(v->u64);
			case configTypeEnum::Float:  return numeric_cast<float>(v->f);
			case configTypeEnum::Double: return numeric_cast<float>(v->d);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toFloat();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const double cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return numeric_cast<double>(v->s32);
			case configTypeEnum::Uint32: return numeric_cast<double>(v->u32);
			case configTypeEnum::Sint64: return numeric_cast<double>(v->s64);
			case configTypeEnum::Uint64: return numeric_cast<double>(v->u64);
			case configTypeEnum::Float:  return numeric_cast<double>(v->f);
			case configTypeEnum::Double: return numeric_cast<double>(v->d);
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return v->s->toDouble();
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}
		template<> const string cast(const variable *v)
		{
			switch (v->type)
			{
			case configTypeEnum::Bool: return v->b;
			case configTypeEnum::Sint32: return v->s32;
			case configTypeEnum::Uint32: return v->u32;
			case configTypeEnum::Sint64: return v->s64;
			case configTypeEnum::Uint64: return v->u64;
			case configTypeEnum::Float: return v->f;
			case configTypeEnum::Double: return v->d;
			case configTypeEnum::String: CAGE_ASSERT_RUNTIME(v->s); return *v->s;
			default: CAGE_THROW_ERROR(exception, "variable type is unspecified");
			}
		}

		class configListImpl : public configListClass
		{
			scopeLock<mutexClass> lock;

		public:
			varsType::iterator it, et;
			bool valid;

			configListImpl() : lock(mut())
			{
				it = vars().begin();
				et = vars().end();
				valid = it != et;
			}

			void next()
			{
				CAGE_ASSERT_RUNTIME(valid, "config variables lister is at invalid location");
				valid = ++it != et;
			}
		};
	}

	void configSetBool(const string &name, bool value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetSint32(const string &name, sint32 value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetUint32(const string &name, uint32 value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetSint64(const string &name, sint64 value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetUint64(const string &name, uint64 value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetFloat(const string &name, float value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetDouble(const string &name, double value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetString(const string &name, const string &value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->set(value);
	}

	void configSetDynamic(const string &name, const string &value)
	{
		if (value.isInteger(false)) return configSetUint64(name, value.toUint64());
		if (value.isInteger(true)) return configSetSint64(name, value.toSint64());
		if (value.isReal(true)) return configSetFloat(name, value.toFloat());
		if (value.isBool()) return configSetBool(name, value.toBool());
		configSetString(name, value);
	}

	bool configGetBool(const string &name, bool value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<bool>(v);
	}

	sint32 configGetSint32(const string &name, sint32 value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<sint32>(v);
	}

	uint32 configGetUint32(const string &name, uint32 value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<uint32>(v);
	}

	sint64 configGetSint64(const string &name, sint64 value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<sint64>(v);
	}

	uint64 configGetUint64(const string &name, uint64 value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<uint64>(v);
	}

	float configGetFloat(const string &name, float value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<float>(v);
	}

	double configGetDouble(const string &name, double value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<double>(v);
	}

	string configGetString(const string &name, const string &value)
	{
		scopeLock<mutexClass> lock(mut());
		variable *v = getVar(name);
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
		return cast<string>(v);
	}

	configBool::configBool(const string &name, bool value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configBool::operator bool() const
	{
		return cast<bool>((variable*)data);
	}

	configBool & configBool::operator=(bool value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configSint32::configSint32(const string &name, sint32 value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configSint32::operator sint32() const
	{
		return cast<sint32>((variable*)data);
	}

	configSint32 & configSint32::operator=(sint32 value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configUint32::configUint32(const string &name, uint32 value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configUint32::operator uint32() const
	{
		return cast<uint32>((variable*)data);
	}

	configUint32 & configUint32::operator=(uint32 value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configSint64::configSint64(const string &name, sint64 value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configSint64::operator sint64() const
	{
		return cast<sint64>((variable*)data);
	}

	configSint64 & configSint64::operator=(sint64 value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configUint64::configUint64(const string &name, uint64 value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configUint64::operator uint64() const
	{
		return cast<uint64>((variable*)data);
	}

	configUint64 & configUint64::operator=(uint64 value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configFloat::configFloat(const string &name, float value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configFloat::operator float() const
	{
		return cast<float>((variable*)data);
	}

	configFloat & configFloat::operator=(float value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configDouble::configDouble(const string &name, double value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configDouble::operator double() const
	{
		return cast<double>((variable*)data);
	}

	configDouble & configDouble::operator=(double value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	configString::configString(const string &name, const string &value)
	{
		scopeLock<mutexClass> lock(mut());
		data = getVar(name);
		variable *v = (variable*)data;
		if (v->type == configTypeEnum::Undefined)
			v->set(value);
	}

	configString::operator string() const
	{
		return cast<string>((variable*)data);
	}

	configString & configString::operator=(const string &value)
	{
		((variable*)data)->set(value);
		return *this;
	}

	bool configListClass::valid() const
	{
		configListImpl *impl = (configListImpl*)this;
		return impl->valid;
	}

	string configListClass::name() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return impl->it->first;
	}

	configTypeEnum configListClass::type() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return impl->it->second.get()->type;
	}

	string configListClass::typeName() const
	{
		switch (type())
		{
		case configTypeEnum::Bool: return "bool";
		case configTypeEnum::Sint32: return "sint32";
		case configTypeEnum::Uint32: return "uint32";
		case configTypeEnum::Sint64: return "sint64";
		case configTypeEnum::Uint64: return "uint64";
		case configTypeEnum::Float: return "float";
		case configTypeEnum::Double: return "double";
		case configTypeEnum::String: return "string";
		default: return "unspecified";
		}
	}

	bool configListClass::getBool() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<bool>(impl->it->second.get());
	}

	sint32 configListClass::getSint32() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<sint32>(impl->it->second.get());
	}

	uint32 configListClass::getUint32() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<uint32>(impl->it->second.get());
	}

	sint64 configListClass::getSint64() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<sint64>(impl->it->second.get());
	}

	uint64 configListClass::getUint64() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<uint64>(impl->it->second.get());
	}

	float configListClass::getFloat() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<float>(impl->it->second.get());
	}

	double configListClass::getDouble() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<double>(impl->it->second.get());
	}

	string configListClass::getString() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "config variables lister is at invalid location");
		return cast<string>(impl->it->second.get());
	}

	void configListClass::next()
	{
		configListImpl *impl = (configListImpl*)this;
		impl->next();
	}

	holder<configListClass> newConfigList()
	{
		return detail::systemArena().createImpl <configListClass, configListImpl>();
	}

	void configLoadIni(const string &filename, const string &prefix)
	{
		if (prefix.pattern("", ".", "") || prefix.empty())
			CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config prefix '" + prefix + "'");
		holder<iniClass> ini = newIni();
		ini->load(filename);
		for (uint32 sectionIndex = 0, sectionIndexEnd = ini->sectionCount(); sectionIndex != sectionIndexEnd; sectionIndex++)
		{
			string section = ini->section(sectionIndex);
			if (section.pattern("", ".", ""))
				CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config section '" + section + "'");
			for (uint32 index = 0, indexEnd = ini->itemCount(section); index != indexEnd; index++)
			{
				string name = ini->item(section, index);
				if (name.pattern("", ".", ""))
					CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config field '" + name + "'");
				string value = ini->getString(section, name);
				configSetDynamic(string() + (prefix.empty() ? "" : prefix + ".") + section + "." + name, value);
			}
		}
	}

	void configSaveIni(const string &filename, const string &prefix)
	{
		holder<iniClass> ini = newIni();
		holder<configListClass> cnf = newConfigList();
		while (cnf->valid())
		{
			string p, s;
			string name = cnf->name();
			p = name.split(".");
			s = name.split(".");
			if (p == prefix)
				ini->setString(s, name, cnf->getString());
			cnf->next();
		}
		ini->save(filename);
	}
}