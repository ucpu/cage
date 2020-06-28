#ifndef guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
#define guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F

#include "core.h"

namespace cage
{
	// configuration is held in global memory
	// creating configuration variables is thread safe
	// modifying configuration variables is _not_ thread safe
	// configuration variables, once created, cannot be removed

	// structures Config* store a reference to the config variable and are very fast to access, but are not thread safe
	// functions ConfigSet* and ConfigGet* are simpler to use for one-time access

	// assigning a value to config variable also changes the type of the variable
	// reading a value from config variable converts the value from the type of the variable to the requested type, and does not change the type of the variable itself

	enum class ConfigTypeEnum : uint32
	{
		Undefined,
		Bool,
		Sint32,
		Uint32,
		Sint64,
		Uint64,
		Float,
		Double,
		String,
	};

	CAGE_CORE_API string configTypeToString(const ConfigTypeEnum type);
	CAGE_CORE_API void configSetDynamic(const string &name, const string &value); // changes the type of the config variable to the one best suited for the value
	CAGE_CORE_API ConfigTypeEnum configGetType(const string &name);

#define GCHL_CONFIG(T, t) \
	struct CAGE_CORE_API Config##T \
	{ \
		explicit Config##T(const string &name); \
		explicit Config##T(const string &name, t default_); \
		operator t() const; \
		Config##T &operator = (t value); \
		private: void *data; \
	}; \
	CAGE_CORE_API void configSet##T(const string &name, t value); \
	CAGE_CORE_API t configGet##T(const string &name, t default_ = 0);
	GCHL_CONFIG(Bool, bool)
	GCHL_CONFIG(Sint32, sint32)
	GCHL_CONFIG(Sint64, sint64)
	GCHL_CONFIG(Uint32, uint32)
	GCHL_CONFIG(Uint64, uint64)
	GCHL_CONFIG(Float, float)
	GCHL_CONFIG(Double, double)
#undef GCHL_CONFIG

	struct CAGE_CORE_API ConfigString
	{
		explicit ConfigString(const string &name);
		explicit ConfigString(const string &name, const string &default_);
		operator string() const;
		ConfigString &operator = (const string &value);
	private:
		void *data;
	};
	CAGE_CORE_API void configSetString(const string &name, const string &value);
	CAGE_CORE_API string configGetString(const string &name, const string &default_ = "");

	class CAGE_CORE_API ConfigList : private Immovable
	{
	public:
		bool valid() const;
		string name() const;
		ConfigTypeEnum type() const;
		string typeName() const;
		bool getBool() const;
		sint32 getSint32() const;
		uint32 getUint32() const;
		sint64 getSint64() const;
		uint64 getUint64() const;
		float getFloat() const;
		double getDouble() const;
		string getString() const;
		void next();
	};

	CAGE_CORE_API Holder<ConfigList> newConfigList();

	// ini values are mapped to config variables as 'prefix/section/item = value'

	CAGE_CORE_API void configApplyIni(const Ini *ini, const string &prefix);
	CAGE_CORE_API Holder<Ini> configGenerateIni(const string &prefix);
	CAGE_CORE_API void configImportIni(const string &filename, const string &prefix);
	CAGE_CORE_API void configExportIni(const string &filename, const string &prefix);

	namespace detail
	{
		CAGE_CORE_API string getConfigAppPrefix();
	}
}

#endif // guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
