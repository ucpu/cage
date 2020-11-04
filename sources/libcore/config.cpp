#include <cage-core/string.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/files.h>
#include <cage-core/config.h>
#include <cage-core/ini.h>
#include <cage-core/debug.h>
#include <cage-core/macros.h>

#include <map>
#include <vector>

namespace cage
{
	namespace
	{
		Mutex *mut()
		{
			static Holder<Mutex> *m = new Holder<Mutex>(newMutex()); // this leak is intentional
			return m->get();
		}

		struct Variable
		{
			ConfigTypeEnum type;
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
			string *s; // pointer is separate from the union to prevent memory corruption

			Variable() : type(ConfigTypeEnum::Undefined), u64(0), s(nullptr) {}

			void set(bool value) { setType(ConfigTypeEnum::Bool); b = value; }
			void set(sint32 value) { setType(ConfigTypeEnum::Sint32); s32 = value; }
			void set(uint32 value) { setType(ConfigTypeEnum::Uint32); u32 = value; }
			void set(sint64 value) { setType(ConfigTypeEnum::Sint64); s64 = value; }
			void set(uint64 value) { setType(ConfigTypeEnum::Uint64); u64 = value; }
			void set(float value) { setType(ConfigTypeEnum::Float); f = value; }
			void set(double value) { setType(ConfigTypeEnum::Double); d = value; }
			void set(const string &value) { if (!s) s = detail::systemArena().createObject<string>(value); else *s = value; setType(ConfigTypeEnum::String); }
			void setDynamic(const string &value)
			{
				if (value.empty())
					set(value);
				else if (isDigitsOnly(value))
					set(toUint64(value));
				else if (isInteger(value))
					set(toSint64(value));
				else if (isReal(value))
					set(toFloat(value));
				else if (isBool(value))
					set(toBool(value));
				else
					set(value);
			}

		private:
			void setType(ConfigTypeEnum t)
			{
				CAGE_ASSERT(t != ConfigTypeEnum::Undefined);
				if (t != type && type != ConfigTypeEnum::Undefined)
					CAGE_LOG(SeverityEnum::Warning, "config", "changing type of config variable");
				type = t;
			}
		};

		typedef std::map<string, Variable*, StringComparatorFast> VarsType;

		VarsType &directVariables()
		{
			static VarsType *v = new VarsType(); // intentionally left to leak
			return *v;
		}

		Variable *directVariable(const string &name)
		{
			CAGE_ASSERT(!name.empty());
			if (find(name, ".") != m)
			{
				CAGE_LOG(SeverityEnum::Warning, "config", stringizer() + "accessing deprecated config variable '" + name + "'");
				CAGE_LOG(SeverityEnum::Note, "config", "new names use slashes instead of dots");
				detail::debugBreakpoint();
			}
			Variable *v = directVariables()[name];
			if (!v)
			{
				directVariables()[name] = detail::systemArena().createObject<Variable>();
				v = directVariables()[name];
			}
			return v;
		}

		void loadConfigFile(const string &filename, const string &prefix)
		{
			CAGE_LOG_DEBUG(SeverityEnum::Info, "config", stringizer() + "trying to load configuration file: '" + filename + "'");
			if (pathIsFile(filename))
			{
				CAGE_LOG(SeverityEnum::Info, "config", stringizer() + "loading configuration file: '" + filename + "'");
				string pref = prefix;
				if (!pref.empty())
					pref += "/";
				// the logic of function configLoadIni is replicated here, but we are inside the mutex already
				try
				{
					Holder<Ini> ini = newIni();
					ini->importFile(filename);
					for (const string &section : ini->sections())
					{
						for (const string &name : ini->items(section))
						{
							string value = ini->getString(section, name);
							directVariable(pref + section + "/" + name)->setDynamic(value);
						}
					}
				}
				catch(...)
				{
					// do nothing
				}
			}
		}

		int loadGlobalConfiguration()
		{
			string pr = detail::getConfigAppPrefix();
			string ep = pathExtractDirectory(detail::getExecutableFullPath());
			string wp = pathWorkingDir();
			bool same = ep == wp;
			if (!same)
				loadConfigFile(pathJoin(ep, "cage.ini"), "");
			loadConfigFile(pathJoin(wp, "cage.ini"), "");
			if (!same)
				loadConfigFile(pathJoin(ep, pr + ".ini"), pr);
			loadConfigFile(pathJoin(wp, pr + ".ini"), pr);
			return 0;
		}

		Variable *getVar(const string &name)
		{
			static int cageIni = loadGlobalConfiguration();
			(void)cageIni;
			return directVariable(name);
		}

		template<class C>
		C cast(const Variable *v);

		template<>
		bool cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return v->s32 != 0;
			case ConfigTypeEnum::Uint32: return v->u32 != 0;
			case ConfigTypeEnum::Sint64: return v->s64 != 0;
			case ConfigTypeEnum::Uint64: return v->u64 != 0;
			case ConfigTypeEnum::Float: return real(v->f) != real(0);
			case ConfigTypeEnum::Double: return real(v->d) != real(0);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toBool(*v->s);
			default: return false;
			}
		}

		template<>
		sint32 cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return numeric_cast<sint32>(v->s32);
			case ConfigTypeEnum::Uint32: return numeric_cast<sint32>(v->u32);
			case ConfigTypeEnum::Sint64: return numeric_cast<sint32>(v->s64);
			case ConfigTypeEnum::Uint64: return numeric_cast<sint32>(v->u64);
			case ConfigTypeEnum::Float:  return numeric_cast<sint32>(v->f);
			case ConfigTypeEnum::Double: return numeric_cast<sint32>(v->d);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toSint32(*v->s);
			default: return 0;
			}
		}

		template<>
		uint32 cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return numeric_cast<uint32>(v->s32);
			case ConfigTypeEnum::Uint32: return numeric_cast<uint32>(v->u32);
			case ConfigTypeEnum::Sint64: return numeric_cast<uint32>(v->s64);
			case ConfigTypeEnum::Uint64: return numeric_cast<uint32>(v->u64);
			case ConfigTypeEnum::Float:  return numeric_cast<uint32>(v->f);
			case ConfigTypeEnum::Double: return numeric_cast<uint32>(v->d);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toUint32(*v->s);
			default: return 0;
			}
		}

		template<>
		sint64 cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return numeric_cast<sint64>(v->s32);
			case ConfigTypeEnum::Uint32: return numeric_cast<sint64>(v->u32);
			case ConfigTypeEnum::Sint64: return numeric_cast<sint64>(v->s64);
			case ConfigTypeEnum::Uint64: return numeric_cast<sint64>(v->u64);
			case ConfigTypeEnum::Float:  return numeric_cast<sint64>(v->f);
			case ConfigTypeEnum::Double: return numeric_cast<sint64>(v->d);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toSint64(*v->s);
			default: return 0;
			}
		}

		template<>
		uint64 cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return numeric_cast<uint64>(v->s32);
			case ConfigTypeEnum::Uint32: return numeric_cast<uint64>(v->u32);
			case ConfigTypeEnum::Sint64: return numeric_cast<uint64>(v->s64);
			case ConfigTypeEnum::Uint64: return numeric_cast<uint64>(v->u64);
			case ConfigTypeEnum::Float:  return numeric_cast<uint64>(v->f);
			case ConfigTypeEnum::Double: return numeric_cast<uint64>(v->d);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toUint64(*v->s);
			default: return 0;
			}
		}

		template<>
		float cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return numeric_cast<float>(v->s32);
			case ConfigTypeEnum::Uint32: return numeric_cast<float>(v->u32);
			case ConfigTypeEnum::Sint64: return numeric_cast<float>(v->s64);
			case ConfigTypeEnum::Uint64: return numeric_cast<float>(v->u64);
			case ConfigTypeEnum::Float:  return numeric_cast<float>(v->f);
			case ConfigTypeEnum::Double: return numeric_cast<float>(v->d);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toFloat(*v->s);
			default: return 0;
			}
		}

		template<>
		double cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return v->b;
			case ConfigTypeEnum::Sint32: return numeric_cast<double>(v->s32);
			case ConfigTypeEnum::Uint32: return numeric_cast<double>(v->u32);
			case ConfigTypeEnum::Sint64: return numeric_cast<double>(v->s64);
			case ConfigTypeEnum::Uint64: return numeric_cast<double>(v->u64);
			case ConfigTypeEnum::Float:  return numeric_cast<double>(v->f);
			case ConfigTypeEnum::Double: return numeric_cast<double>(v->d);
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return toDouble(*v->s);
			default: return 0;
			}
		}

		template<>
		string cast(const Variable *v)
		{
			switch (v->type)
			{
			case ConfigTypeEnum::Bool: return stringizer() + v->b;
			case ConfigTypeEnum::Sint32: return stringizer() + v->s32;
			case ConfigTypeEnum::Uint32: return stringizer() + v->u32;
			case ConfigTypeEnum::Sint64: return stringizer() + v->s64;
			case ConfigTypeEnum::Uint64: return stringizer() + v->u64;
			case ConfigTypeEnum::Float: return stringizer() + v->f;
			case ConfigTypeEnum::Double: return stringizer() + v->d;
			case ConfigTypeEnum::String: CAGE_ASSERT(v->s); return *v->s;
			default: return "";
			}
		}

		class ConfigListImpl : public ConfigList
		{
		public:
			std::vector<string> names;
			Variable *var;
			uint32 index;
			bool valid;

			ConfigListImpl() : var(nullptr), index(0), valid(false)
			{
				ScopeLock<Mutex> lock(mut());
				const auto &mp = directVariables();
				names.reserve(mp.size());
				for (const auto &it : mp)
					names.push_back(it.first);
				valid = !names.empty();
				if (valid)
					var = getVar(names[0]);
			}

			void next()
			{
				CAGE_ASSERT(valid);
				index++;
				valid = index < names.size();
				if (valid)
				{
					ScopeLock<Mutex> lock(mut());
					var = getVar(names[index]);
				}
				else
					var = nullptr;
			}
		};
	}

	string configTypeToString(const ConfigTypeEnum type)
	{
		switch (type)
		{
		case ConfigTypeEnum::Bool: return "bool";
		case ConfigTypeEnum::Sint32: return "sint32";
		case ConfigTypeEnum::Uint32: return "uint32";
		case ConfigTypeEnum::Sint64: return "sint64";
		case ConfigTypeEnum::Uint64: return "uint64";
		case ConfigTypeEnum::Float: return "float";
		case ConfigTypeEnum::Double: return "double";
		case ConfigTypeEnum::String: return "string";
		case ConfigTypeEnum::Undefined: return "undefined";
		default: CAGE_THROW_CRITICAL(Exception, "invalid config type enum");
		}
	}

	void configSetDynamic(const string &name, const string &value)
	{
		ScopeLock<Mutex> lock(mut());
		getVar(name)->setDynamic(value);
	}

	ConfigTypeEnum configGetType(const string &name)
	{
		ScopeLock<Mutex> lock(mut());
		return getVar(name)->type;
	}

#define GCHL_CONFIG(T, t) \
	void CAGE_JOIN(configSet, T)(const string &name, t value) { ScopeLock<Mutex> lock(mut()); getVar(name)->set(value); } \
	t CAGE_JOIN(configGet, T)(const string &name, t default_) { ScopeLock<Mutex> lock(mut()); Variable *v = getVar(name); if (v->type == ConfigTypeEnum::Undefined) return default_; return cast<t>(v); } \
	CAGE_JOIN(Config, T)::CAGE_JOIN(Config, T)(const string &name) { ScopeLock<Mutex> lock(mut()); data = getVar(name); } \
	CAGE_JOIN(Config, T)::CAGE_JOIN(Config, T)(const string &name, t default_) { ScopeLock<Mutex> lock(mut()); data = getVar(name); Variable *v = (Variable*)data; if (v->type == ConfigTypeEnum::Undefined) v->set(default_); } \
	CAGE_JOIN(Config, T) &CAGE_JOIN(Config, T)::operator = (t value) { ((Variable*)data)->set(value); return *this; } \
	CAGE_JOIN(Config, T)::operator t() const { return cast<t>((Variable*)data); } \
	t ConfigList::CAGE_JOIN(get, T)() const { ConfigListImpl *impl = (ConfigListImpl*)this; CAGE_ASSERT(impl->valid); return cast<t>(impl->var); }
	GCHL_CONFIG(Bool, bool)
	GCHL_CONFIG(Sint32, sint32)
	GCHL_CONFIG(Sint64, sint64)
	GCHL_CONFIG(Uint32, uint32)
	GCHL_CONFIG(Uint64, uint64)
	GCHL_CONFIG(Float, float)
	GCHL_CONFIG(Double, double)
#undef GCHL_CONFIG

	void configSetString(const string &name, const string &value) { ScopeLock<Mutex> lock(mut()); getVar(name)->set(value); }
	string configGetString(const string &name, const string &default_) { ScopeLock<Mutex> lock(mut()); Variable *v = getVar(name); if (v->type == ConfigTypeEnum::Undefined) return default_; return cast<string>(v); }
	ConfigString::ConfigString(const string &name) { ScopeLock<Mutex> lock(mut()); data = getVar(name); }
	ConfigString::ConfigString(const string &name, const string &default_) { ScopeLock<Mutex> lock(mut()); data = getVar(name); Variable *v = (Variable*)data; if (v->type == ConfigTypeEnum::Undefined) v->set(default_); }
	ConfigString &ConfigString::operator = (const string &value) { ((Variable*)data)->set(value); return *this; }
	ConfigString::operator string() const { return cast<string>((Variable*)data); }
	string ConfigList::getString() const { ConfigListImpl *impl = (ConfigListImpl*)this; CAGE_ASSERT(impl->valid); return cast<string>(impl->var); }

	bool ConfigList::valid() const
	{
		ConfigListImpl *impl = (ConfigListImpl*)this;
		return impl->valid;
	}

	string ConfigList::name() const
	{
		ConfigListImpl *impl = (ConfigListImpl*)this;
		CAGE_ASSERT(impl->valid);
		return impl->names[impl->index];
	}

	ConfigTypeEnum ConfigList::type() const
	{
		ConfigListImpl *impl = (ConfigListImpl*)this;
		CAGE_ASSERT(impl->valid);
		return impl->var->type;
	}

	string ConfigList::typeName() const
	{
		return configTypeToString(type());
	}

	void ConfigList::next()
	{
		ConfigListImpl *impl = (ConfigListImpl*)this;
		impl->next();
	}

	Holder<ConfigList> newConfigList()
	{
		return detail::systemArena().createImpl<ConfigList, ConfigListImpl>();
	}

	void configApplyIni(const Ini *ini, const string &prefix)
	{
		if (find(prefix, '/') != m || prefix.empty())
			CAGE_LOG(SeverityEnum::Warning, "config", stringizer() + "dangerous config prefix '" + prefix + "'");
		string pref = prefix.empty() ? "" : prefix + "/";
		for (const string &section : ini->sections())
		{
			if (prefix.empty() && find(section, '/') != m)
				CAGE_LOG(SeverityEnum::Warning, "config", stringizer() + "dangerous config section '" + section + "'");
			for (const string &name : ini->items(section))
			{
				if (prefix.empty() && find(name, '/') != m)
					CAGE_LOG(SeverityEnum::Warning, "config", stringizer() + "dangerous config field '" + name + "'");
				string value = ini->getString(section, name);
				configSetDynamic(stringizer() + pref + section + "/" + name, value);
			}
		}
	}

	Holder<Ini> configGenerateIni(const string &prefix)
	{
		if (find(prefix, '/') != m || prefix.empty())
			CAGE_LOG(SeverityEnum::Warning, "config", stringizer() + "dangerous config prefix '" + prefix + "'");
		Holder<Ini> ini = newIni();
		Holder<ConfigList> cnf = newConfigList();
		while (cnf->valid())
		{
			string p = reverse(cnf->name());
			string n = reverse(split(p, "/"));
			string s = reverse(split(p, "/"));
			p = reverse(p);
			if (prefix.empty())
				ini->set(p + "/" + s, n, cnf->getString());
			else if (p == prefix)
				ini->set(s, n, cnf->getString());
			cnf->next();
		}
		return ini;
	}

	void configImportIni(const string &filename, const string &prefix)
	{
		Holder<Ini> ini = newIni();
		ini->importFile(filename);
		configApplyIni(ini.get(), prefix);
	}

	void configExportIni(const string &filename, const string &prefix)
	{
		Holder<Ini> ini = configGenerateIni(prefix);
		ini->exportFile(filename);
	}

	namespace detail
	{
		string getConfigAppPrefix()
		{
			return pathExtractFilename(detail::getExecutableFullPathNoExe());
		}
	}

	namespace
	{
		ConfigBool confAutoSave("cage/config/autoSave", false);

		struct AutoSaveConfig
		{
			~AutoSaveConfig()
			{
				if (confAutoSave)
				{
					try
					{
						configExportIni(pathExtractFilename(detail::getExecutableFullPathNoExe()) + ".ini", detail::getConfigAppPrefix());
					}
					catch (...)
					{
						CAGE_LOG(SeverityEnum::Warning, "config", "failed to save configuration");
					}
				}
			}
		} autoSaveConfigInstance;
	}
}

