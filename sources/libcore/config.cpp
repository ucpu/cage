#include <variant>
#include <vector>

#include <unordered_dense.h>

#include <cage-core/concurrent.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/logger.h>
#include <cage-core/macros.h>
#include <cage-core/math.h>
#include <cage-core/stdHash.h>
#include <cage-core/string.h>

namespace cage
{
	PathTypeFlags realType(const String &path);
	Holder<File> realNewFile(const String &path, const FileMode &mode);
	StringPointer configTypeToString(const ConfigTypeEnum type)
	{
		switch (type)
		{
			case ConfigTypeEnum::Bool:
				return "bool";
			case ConfigTypeEnum::Sint32:
				return "sint32";
			case ConfigTypeEnum::Uint32:
				return "uint32";
			case ConfigTypeEnum::Sint64:
				return "sint64";
			case ConfigTypeEnum::Uint64:
				return "uint64";
			case ConfigTypeEnum::Float:
				return "float";
			case ConfigTypeEnum::Double:
				return "double";
			case ConfigTypeEnum::String:
				return "string";
			case ConfigTypeEnum::Undefined:
				return "undefined";
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid config type enum");
		}
	}

	using privat::ConfigVariable;

	// variables and storage

	namespace privat
	{
		struct ConfigVariable : private Immovable
		{
			std::variant<std::monostate, bool, sint32, uint32, sint64, uint64, float, double, String> data;
			String name;
			ConfigTypeEnum type = ConfigTypeEnum::Undefined;

			ConfigVariable(const String &name) : name(name) {}
		};
	}

	namespace
	{
		int loadGlobalConfiguration();

		Mutex *mut()
		{
			static Holder<Mutex> *m = new Holder<Mutex>(newMutex()); // this leak is intentional
			return +*m;
		}

		struct Hasher
		{
			auto operator()(const ConfigVariable *const ptr) const { return ankerl::unordered_dense::hash<String>()(ptr->name); }
		};

		struct Comparator
		{
			auto operator()(const ConfigVariable *const a, const ConfigVariable *const b) const { return a->name == b->name; }
		};

		using ConfigStorage = ankerl::unordered_dense::set<ConfigVariable *, Hasher, Comparator>;

		ConfigStorage &storageAlreadyLocked()
		{
			static ConfigStorage *v = new ConfigStorage(); // this leak is intentional
			return *v;
		}

		ConfigVariable *cfgVarAlreadyLocked(const String &name)
		{
			ConfigStorage &s = storageAlreadyLocked();
			ConfigVariable tmp(name);
			auto it = s.find(&tmp);
			if (it != s.end())
				return *it;
			return *s.insert(new ConfigVariable(name)).first; // this leak is intentional
		}

		ConfigVariable *cfgVar(const String &name)
		{
			ScopeLock lock(mut());
			static int dummy = loadGlobalConfiguration();
			(void)dummy;
			return cfgVarAlreadyLocked(name);
		}

		template<class T>
		struct TypeMapping;
		template<>
		struct TypeMapping<bool>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Bool;
		};
		template<>
		struct TypeMapping<sint32>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Sint32;
		};
		template<>
		struct TypeMapping<uint32>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Uint32;
		};
		template<>
		struct TypeMapping<sint64>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Sint64;
		};
		template<>
		struct TypeMapping<uint64>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Uint64;
		};
		template<>
		struct TypeMapping<float>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Float;
		};
		template<>
		struct TypeMapping<double>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::Double;
		};
		template<>
		struct TypeMapping<String>
		{
			static constexpr ConfigTypeEnum type = ConfigTypeEnum::String;
		};

		template<class T>
		void cfgSet(ConfigVariable *cfg, const T &value)
		{
			cfg->type = ConfigTypeEnum::Undefined;
			cfg->data = value;
			cfg->type = TypeMapping<T>::type;
		}

		void cfgSetDynamic(ConfigVariable *cfg, const String &value)
		{
			if (value.empty())
				cfgSet(cfg, value);
			else if (isDigitsOnly(value))
			{
				try
				{
					detail::OverrideException oe;
					cfgSet(cfg, toUint32(value));
				}
				catch (const cage::Exception &)
				{
					cfgSet(cfg, toUint64(value));
				}
			}
			else if (isInteger(value))
			{
				try
				{
					detail::OverrideException oe;
					cfgSet(cfg, toSint32(value));
				}
				catch (const cage::Exception &)
				{
					cfgSet(cfg, toSint64(value));
				}
			}
			else if (isReal(value))
				cfgSet(cfg, toFloat(value));
			else if (isBool(value))
				cfgSet(cfg, toBool(value));
			else
				cfgSet(cfg, value);
		}

		template<class T>
		ConfigVariable *cfgVar(const String &name, const T &default_)
		{
			ConfigVariable *var = cfgVar(name);
			if (var->type == ConfigTypeEnum::Undefined)
				cfgSet<T>(var, default_);
			return var;
		}

		template<class T>
		T stringToNumber(const String &s);
		template<>
		sint32 stringToNumber(const String &s)
		{
			return toUint32(s);
		}
		template<>
		uint32 stringToNumber(const String &s)
		{
			return toSint32(s);
		}
		template<>
		sint64 stringToNumber(const String &s)
		{
			return toUint64(s);
		}
		template<>
		uint64 stringToNumber(const String &s)
		{
			return toSint64(s);
		}
		template<>
		float stringToNumber(const String &s)
		{
			return toFloat(s);
		}
		template<>
		double stringToNumber(const String &s)
		{
			return toDouble(s);
		}

		template<class T>
		T cfgGet(const ConfigVariable *cfg)
		{
			switch (cfg->type)
			{
				case ConfigTypeEnum::Bool:
					return std::get<bool>(cfg->data);
				case ConfigTypeEnum::Sint32:
					return numeric_cast<T>(std::get<sint32>(cfg->data));
				case ConfigTypeEnum::Uint32:
					return numeric_cast<T>(std::get<uint32>(cfg->data));
				case ConfigTypeEnum::Sint64:
					return numeric_cast<T>(std::get<sint64>(cfg->data));
				case ConfigTypeEnum::Uint64:
					return numeric_cast<T>(std::get<uint64>(cfg->data));
				case ConfigTypeEnum::Float:
					return numeric_cast<T>(std::get<float>(cfg->data));
				case ConfigTypeEnum::Double:
					return numeric_cast<T>(std::get<double>(cfg->data));
				case ConfigTypeEnum::String:
					return stringToNumber<T>(std::get<String>(cfg->data));
				default:
					return 0;
			}
		}

		template<>
		bool cfgGet(const ConfigVariable *cfg)
		{
			switch (cfg->type)
			{
				case ConfigTypeEnum::Bool:
					return std::get<bool>(cfg->data);
				case ConfigTypeEnum::Sint32:
					return !!std::get<sint32>(cfg->data);
				case ConfigTypeEnum::Uint32:
					return !!std::get<uint32>(cfg->data);
				case ConfigTypeEnum::Sint64:
					return !!std::get<sint64>(cfg->data);
				case ConfigTypeEnum::Uint64:
					return !!std::get<uint64>(cfg->data);
				case ConfigTypeEnum::Float:
					return !!std::get<float>(cfg->data);
				case ConfigTypeEnum::Double:
					return !!std::get<double>(cfg->data);
				case ConfigTypeEnum::String:
					return toBool(std::get<String>(cfg->data));
				default:
					return false;
			}
		}

		template<>
		String cfgGet(const ConfigVariable *cfg)
		{
			switch (cfg->type)
			{
				case ConfigTypeEnum::Bool:
					return Stringizer() + std::get<bool>(cfg->data);
				case ConfigTypeEnum::Sint32:
					return Stringizer() + std::get<sint32>(cfg->data);
				case ConfigTypeEnum::Uint32:
					return Stringizer() + std::get<uint32>(cfg->data);
				case ConfigTypeEnum::Sint64:
					return Stringizer() + std::get<sint64>(cfg->data);
				case ConfigTypeEnum::Uint64:
					return Stringizer() + std::get<uint64>(cfg->data);
				case ConfigTypeEnum::Float:
					return Stringizer() + std::get<float>(cfg->data);
				case ConfigTypeEnum::Double:
					return Stringizer() + std::get<double>(cfg->data);
				case ConfigTypeEnum::String:
					return std::get<String>(cfg->data);
				default:
					return "";
			}
		}

		template<class T>
		T cfgGet(const ConfigList *ths);

		template<class T>
		void cfgSet(const String &name, const T &value)
		{
			cfgSet<T>(cfgVar(name), value);
		}

		template<class T>
		T cfgGet(const String &name, const T &default_)
		{
			const ConfigVariable *cfg = cfgVar(name);
			if (cfg->type == ConfigTypeEnum::Undefined)
				return default_;
			return cfgGet<T>(cfg);
		}
	}

	// public api

	void configSetString(const String &name, const String &value)
	{
		cfgSet(name, value);
	}
	String configGetString(const String &name, const String &default_)
	{
		return cfgGet(name, default_);
	}
	ConfigString::ConfigString(const String &name)
	{
		data = cfgVar(name);
	}
	ConfigString::ConfigString(const String &name, const String &default_)
	{
		data = cfgVar(name, default_);
	}
	ConfigString &ConfigString::operator=(const String &value)
	{
		cfgSet(data, value);
		return *this;
	}
	const String &ConfigString::name() const
	{
		return data->name;
	}
	ConfigString::operator String() const
	{
		return cfgGet<String>(data);
	}
	String ConfigList::getString() const
	{
		return cfgGet<String>(this);
	}

#define GCHL_CONFIG(T, t) \
	void CAGE_JOIN(configSet, T)(const String &name, t value) \
	{ \
		cfgSet(name, value); \
	} \
	t CAGE_JOIN(configGet, T)(const String &name, t default_) \
	{ \
		return cfgGet(name, default_); \
	} \
	CAGE_JOIN(Config, T)::CAGE_JOIN(Config, T)(const String &name) \
	{ \
		data = cfgVar(name); \
	} \
	CAGE_JOIN(Config, T)::CAGE_JOIN(Config, T)(const String &name, t default_) \
	{ \
		data = cfgVar(name, default_); \
	} \
	CAGE_JOIN(Config, T) & CAGE_JOIN(Config, T)::operator=(t value) \
	{ \
		cfgSet(data, value); \
		return *this; \
	} \
	const String &CAGE_JOIN(Config, T)::name() const \
	{ \
		return data->name; \
	} \
	CAGE_JOIN(Config, T)::operator t() const \
	{ \
		return cfgGet<t>(data); \
	} \
	t ConfigList::CAGE_JOIN(get, T)() const \
	{ \
		return cfgGet<t>(this); \
	}
	GCHL_CONFIG(Bool, bool)
	GCHL_CONFIG(Sint32, sint32)
	GCHL_CONFIG(Sint64, sint64)
	GCHL_CONFIG(Uint32, uint32)
	GCHL_CONFIG(Uint64, uint64)
	GCHL_CONFIG(Float, float)
	GCHL_CONFIG(Double, double)
#undef GCHL_CONFIG

	void configSetDynamic(const String &name, const String &value)
	{
		cfgSetDynamic(cfgVar(name), value);
	}

	ConfigTypeEnum configGetType(const String &name)
	{
		return cfgVar(name)->type;
	}

	// ini

	void configApplyIni(const Ini *ini, const String &prefix)
	{
		if (find(prefix, '/') != m || prefix.empty())
			CAGE_LOG(SeverityEnum::Warning, "config", Stringizer() + "dangerous config prefix: " + prefix);
		const String pref = prefix.empty() ? "" : prefix + "/";
		for (const String &section : ini->sections())
		{
			if (prefix.empty() && find(section, '/') != m)
				CAGE_LOG(SeverityEnum::Warning, "config", Stringizer() + "dangerous config section: " + section);
			for (const String &name : ini->items(section))
			{
				if (prefix.empty() && find(name, '/') != m)
					CAGE_LOG(SeverityEnum::Warning, "config", Stringizer() + "dangerous config field: " + name);
				const String value = ini->getString(section, name);
				configSetDynamic(Stringizer() + pref + section + "/" + name, value);
			}
		}
	}

	Holder<Ini> configGenerateIni(const String &prefix)
	{
		if (find(prefix, '/') != m || prefix.empty())
			CAGE_LOG(SeverityEnum::Warning, "config", Stringizer() + "dangerous config prefix: " + prefix);
		Holder<Ini> ini = newIni();
		Holder<ConfigList> cnf = newConfigList();
		while (cnf->valid())
		{
			String p = reverse(cnf->name());
			String n = reverse(split(p, "/"));
			String s = reverse(split(p, "/"));
			p = reverse(p);
			if (prefix.empty())
				ini->set(p + "/" + s, n, cnf->getString());
			else if (p == prefix)
				ini->set(s, n, cnf->getString());
			cnf->next();
		}
		return ini;
	}

	void configImportIni(const String &filename, const String &prefix)
	{
		Holder<Ini> ini = newIni();
		ini->importFile(filename);
		configApplyIni(+ini, prefix);
	}

	void configExportIni(const String &filename, const String &prefix)
	{
		Holder<Ini> ini = configGenerateIni(prefix);
		ini->exportFile(filename);
	}

	// listing

	namespace
	{
		class ConfigListImpl : public ConfigList
		{
		public:
			std::vector<String> names;
			const ConfigVariable *var = nullptr;
			uint32 index = 0;
			bool valid = false;

			ConfigListImpl()
			{
				{
					ScopeLock lock(mut());
					const auto &s = storageAlreadyLocked();
					names.reserve(s.size());
					for (const auto &it : s)
						names.push_back(it->name);
				}
				valid = !names.empty();
				if (valid)
					var = cfgVar(names[0]);
			}

			void next()
			{
				CAGE_ASSERT(valid);
				index++;
				valid = index < names.size();
				if (valid)
					var = cfgVar(names[index]);
				else
					var = nullptr;
			}
		};

		template<class T>
		T cfgGet(const ConfigList *ths)
		{
			const ConfigListImpl *impl = (const ConfigListImpl *)ths;
			CAGE_ASSERT(impl->valid);
			return cfgGet<T>(impl->var);
		}
	}

	bool ConfigList::valid() const
	{
		const ConfigListImpl *impl = (const ConfigListImpl *)this;
		return impl->valid;
	}

	String ConfigList::name() const
	{
		const ConfigListImpl *impl = (const ConfigListImpl *)this;
		CAGE_ASSERT(impl->valid);
		return impl->names[impl->index];
	}

	ConfigTypeEnum ConfigList::type() const
	{
		const ConfigListImpl *impl = (const ConfigListImpl *)this;
		CAGE_ASSERT(impl->valid);
		return impl->var->type;
	}

	StringPointer ConfigList::typeName() const
	{
		return configTypeToString(type());
	}

	void ConfigList::next()
	{
		ConfigListImpl *impl = (ConfigListImpl *)this;
		impl->next();
	}

	Holder<ConfigList> newConfigList()
	{
		return systemMemory().createImpl<ConfigList, ConfigListImpl>();
	}

	// detail

	namespace detail
	{
		String globalConfigPrefix()
		{
			return pathExtractFilename(detail::executableFullPathNoExe());
		}
	}

	// initialization

	namespace
	{
		void loadGlobalConfigFile(const String &filename, const String &prefix)
		{
			CAGE_LOG_DEBUG(SeverityEnum::Info, "config", Stringizer() + "trying to load configuration file: " + filename);
			if (any(realType(filename) & PathTypeFlags::File))
			{
				CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "loading configuration file: " + filename);
				try
				{
					// the logic of function configLoadIni is replicated here, but we are inside the mutex already
					String pref = prefix;
					if (!pref.empty())
						pref += "/";
					Holder<Ini> ini = newIni();
					// real files only to reduce chances of failures
					ini->importBuffer(realNewFile(filename, FileMode(true, false))->readAll());
					for (const String &section : ini->sections())
					{
						for (const String &name : ini->items(section))
						{
							const String value = ini->getString(section, name);
							cfgSetDynamic(cfgVarAlreadyLocked(pref + section + "/" + name), value);
						}
					}
				}
				catch (...)
				{
					// do nothing
				}
			}
		}

		int loadGlobalConfiguration()
		{
			detail::globalLogger(); // ensure global logger was initialized
			const String pr = detail::globalConfigPrefix();
			const String ep = pathExtractDirectory(detail::executableFullPath());
			const String wp = pathWorkingDir();
			const bool same = ep == wp;
			if (!same)
				loadGlobalConfigFile(pathJoin(ep, "cage.ini"), "");
			loadGlobalConfigFile(pathJoin(wp, "cage.ini"), "");
			if (!same)
				loadGlobalConfigFile(pathJoin(ep, pr + ".ini"), pr);
			loadGlobalConfigFile(pathJoin(wp, pr + ".ini"), pr);
			return 0;
		}

		const ConfigBool confAutoSave("cage/config/autoSave", false);

		struct AutoSaveConfig
		{
			~AutoSaveConfig()
			{
				if (confAutoSave)
				{
					try
					{
						configExportIni(pathExtractFilename(detail::executableFullPathNoExe()) + ".ini", detail::globalConfigPrefix());
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
