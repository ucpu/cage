#include <map>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/filesystem.h>
#include <cage-core/ini.h>
#include <cage-core/config.h>

namespace cage
{
	namespace
	{
		mutexClass *mut()
		{
			static holder<mutexClass> *m = new holder<mutexClass>(newMutex()); // this leak is intentional
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
			string *s;

			variable() : type(configTypeEnum::Undefined), u64(0), s(nullptr) {}

			void set(bool value) { type = configTypeEnum::Bool; b = value; }
			void set(sint32 value) { type = configTypeEnum::Sint32; s32 = value; }
			void set(uint32 value) { type = configTypeEnum::Uint32; u32 = value; }
			void set(sint64 value) { type = configTypeEnum::Sint64; s64 = value; }
			void set(uint64 value) { type = configTypeEnum::Uint64; u64 = value; }
			void set(float value) { type = configTypeEnum::Float; f = value; }
			void set(double value) { type = configTypeEnum::Double; d = value; }
			void set(const string &value) { if (!s) s = detail::systemArena().createObject<string>(value); else *s = value; type = configTypeEnum::String; }
			void setDynamic(const string &value)
			{
				if (value.isInteger(false))
					set(value.toUint64());
				else if (value.isInteger(true))
					set(value.toSint64());
				else if (value.isReal(true))
					set(value.toFloat());
				else if (value.isBool())
					set(value.toBool());
				else
					set(value);
			}
		};

		typedef std::map<string, variable*, stringComparatorFast> varsType;

		varsType &directVariables()
		{
			static varsType *v = new varsType(); // intentionally left to leak
			return *v;
		}

		variable *directVariable(const string &name)
		{
			CAGE_ASSERT_RUNTIME(!name.empty(), "variable name cannot be empty");
			variable *v = directVariables()[name];
			if (!v)
			{
				directVariables()[name] = detail::systemArena().createObject<variable>();
				v = directVariables()[name];
			}
			return v;
		}

		bool loadConfigFile(const string &filename, string prefix)
		{
			CAGE_LOG_DEBUG(severityEnum::Info, "config", string() + "trying to load configuration file: '" + filename + "'");
			if (pathIsFile(filename))
			{
				CAGE_LOG(severityEnum::Info, "config", string() + "loading configuration file: '" + filename + "'");
				if (!prefix.empty())
					prefix += ".";
				// the logic of function configLoadIni is replicated here
				try
				{
					holder<iniClass> ini = newIni();
					ini->load(filename);
					for (string section : ini->sections())
					{
						for (string name : ini->items(section))
						{
							string value = ini->getString(section, name);
							directVariable(prefix + section + "." + name)->setDynamic(value);
						}
					}
				}
				catch(...)
				{
					// do nothing
				}
				return true;
			}
			return false;
		}

		int loadGlobalConfiguration()
		{
			string efp = detail::getExecutableFullPathNoExe();
			string en = pathExtractFilename(efp);
			string ep = pathExtractPath(efp);
			string wp = pathWorkingDir();
			if (!loadConfigFile(pathJoin(wp, "cage.ini"), ""))
				loadConfigFile(pathJoin(ep, "cage.ini"), "");
			if (!loadConfigFile(pathJoin(wp, en + ".ini"), en))
				loadConfigFile(pathJoin(ep, en + ".ini"), en);
			return 0;
		}

		variable *getVar(const string &name)
		{
			static int cageIni = loadGlobalConfiguration();
			(void)cageIni;
			return directVariable(name);
		}

		template<class C>
		const C cast(const variable *v)
		{}

		template<>
		const bool cast(const variable *v)
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
			default: return false;
			}
		}

		template<>
		const sint32 cast(const variable *v)
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
			default: return 0;
			}
		}

		template<>
		const uint32 cast(const variable *v)
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
			default: return 0;
			}
		}

		template<>
		const sint64 cast(const variable *v)
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
			default: return 0;
			}
		}

		template<>
		const uint64 cast(const variable *v)
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
			default: return 0;
			}
		}

		template<>
		const float cast(const variable *v)
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
			default: return 0;
			}
		}

		template<>
		const double cast(const variable *v)
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
			default: return 0;
			}
		}

		template<>
		const string cast(const variable *v)
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
			default: return "";
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
				it = directVariables().begin();
				et = directVariables().end();
				valid = it != et;
			}

			void next()
			{
				CAGE_ASSERT_RUNTIME(valid, "configListClass is at invalid location");
				valid = ++it != et;
			}
		};
	}

	string configTypeToString(const configTypeEnum type)
	{
		switch (type)
		{
		case configTypeEnum::Bool: return "bool";
		case configTypeEnum::Sint32: return "sint32";
		case configTypeEnum::Uint32: return "uint32";
		case configTypeEnum::Sint64: return "sint64";
		case configTypeEnum::Uint64: return "uint64";
		case configTypeEnum::Float: return "float";
		case configTypeEnum::Double: return "double";
		case configTypeEnum::String: return "string";
		case configTypeEnum::Undefined: return "undefined";
		default: CAGE_THROW_CRITICAL(exception, "invalid config type enum");
		}
	}

	void configSetDynamic(const string &name, const string &value)
	{
		scopeLock<mutexClass> lock(mut());
		getVar(name)->setDynamic(value);
	}

	configTypeEnum configGetType(const string &name)
	{
		scopeLock<mutexClass> lock(mut());
		return getVar(name)->type;
	}

#define GCHL_CONFIG(T, t) \
	void CAGE_JOIN(configSet, T)(const string &name, t value) { scopeLock<mutexClass> lock(mut()); getVar(name)->set(value); } \
	t CAGE_JOIN(configGet, T)(const string &name, t default_) { scopeLock<mutexClass> lock(mut()); variable *v = getVar(name); if (v->type == configTypeEnum::Undefined) return default_; return cast<t>(v); } \
	CAGE_JOIN(config, T)::CAGE_JOIN(config, T)(const string &name) { scopeLock<mutexClass> lock(mut()); data = getVar(name); } \
	CAGE_JOIN(config, T)::CAGE_JOIN(config, T)(const string &name, t default_) { scopeLock<mutexClass> lock(mut()); data = getVar(name); variable *v = (variable*)data; if (v->type == configTypeEnum::Undefined) v->set(default_); } \
	CAGE_JOIN(config, T) &CAGE_JOIN(config, T)::operator = (t value) { ((variable*)data)->set(value); return *this; } \
	CAGE_JOIN(config, T)::operator t() const { return cast<t>((variable*)data); } \
	t configListClass::CAGE_JOIN(get, T)() const { configListImpl *impl = (configListImpl*)this; CAGE_ASSERT_RUNTIME(impl->valid, "configListClass is at invalid location"); return cast<t>(impl->it->second); }
	GCHL_CONFIG(Bool, bool)
	GCHL_CONFIG(Sint32, sint32)
	GCHL_CONFIG(Sint64, sint64)
	GCHL_CONFIG(Uint32, uint32)
	GCHL_CONFIG(Uint64, uint64)
	GCHL_CONFIG(Float, float)
	GCHL_CONFIG(Double, double)
#undef GCHL_CONFIG

	void configSetString(const string &name, const string &value) { scopeLock<mutexClass> lock(mut()); getVar(name)->set(value); }
	string configGetString(const string &name, const string &default_) { scopeLock<mutexClass> lock(mut()); variable *v = getVar(name); if (v->type == configTypeEnum::Undefined) return default_; return cast<string>(v); }
	configString::configString(const string &name) { scopeLock<mutexClass> lock(mut()); data = getVar(name); }
	configString::configString(const string &name, const string &default_) { scopeLock<mutexClass> lock(mut()); data = getVar(name); variable *v = (variable*)data; if (v->type == configTypeEnum::Undefined) v->set(default_); }
	configString &configString::operator = (const string &value) { ((variable*)data)->set(value); return *this; }
	configString::operator string() const { return cast<string>((variable*)data); }
	string configListClass::getString() const { configListImpl *impl = (configListImpl*)this; CAGE_ASSERT_RUNTIME(impl->valid, "configListClass is at invalid location"); return cast<string>(impl->it->second); }

	bool configListClass::valid() const
	{
		configListImpl *impl = (configListImpl*)this;
		return impl->valid;
	}

	string configListClass::name() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "configListClass is at invalid location");
		return impl->it->first;
	}

	configTypeEnum configListClass::type() const
	{
		configListImpl *impl = (configListImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "configListClass is at invalid location");
		return impl->it->second->type;
	}

	string configListClass::typeName() const
	{
		return configTypeToString(type());
	}

	void configListClass::next()
	{
		configListImpl *impl = (configListImpl*)this;
		impl->next();
	}

	holder<configListClass> newConfigList()
	{
		return detail::systemArena().createImpl<configListClass, configListImpl>();
	}

	void configApplyIni(const iniClass *ini, const string &prefix)
	{
		if (prefix.find('.') != m || prefix.empty())
			CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config prefix '" + prefix + "'");
		for (string section : ini->sections())
		{
			if (section.find('.') != m)
				CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config section '" + section + "'");
			for (string name : ini->items(section))
			{
				if (name.find('.') != m)
					CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config field '" + name + "'");
				string value = ini->getString(section, name);
				configSetDynamic(string() + (prefix.empty() ? "" : prefix + ".") + section + "." + name, value);
			}
		}
	}

	void configGenerateIni(iniClass *ini, const string &prefix)
	{
		if (prefix.find('.') != m || prefix.empty())
			CAGE_LOG(severityEnum::Warning, "config", string() + "dangerous config prefix '" + prefix + "'");
		ini->clear();
		holder<configListClass> cnf = newConfigList();
		while (cnf->valid())
		{
			string p = cnf->name().reverse();
			string n = p.split(".").reverse();
			string s = p.split(".").reverse();
			p = p.reverse();
			if (prefix.empty())
				ini->set(p + "." + s, n, cnf->getString());
			else if (p == prefix)
				ini->set(s, n, cnf->getString());
			cnf->next();
		}
	}

	void configLoadIni(const string &filename, const string &prefix)
	{
		holder<iniClass> ini = newIni();
		ini->load(filename);
		configApplyIni(ini.get(), prefix);
	}

	void configSaveIni(const string &filename, const string &prefix)
	{
		holder<iniClass> ini = newIni();
		configGenerateIni(ini.get(), prefix);
		ini->save(filename);
	}

	namespace
	{
		configBool autoBackup("cage-core.config.autoBackup", false);

		struct autoConfigBackupClass
		{
			~autoConfigBackupClass()
			{
				if (autoBackup)
				{
					try
					{
						configSaveIni(detail::getExecutableFullPathNoExe() + "-backup.ini", "");
					}
					catch (...)
					{
						CAGE_LOG(severityEnum::Warning, "config", "failed to save configuration backup");
					}
				}
			}
		} autoConfigBackupInstance;
	}
}

