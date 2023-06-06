#ifndef guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
#define guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F

#include "core.h"

namespace cage
{
	// configuration is held in global memory
	// creating configuration variables is thread safe
	// modifying configuration variables is _not_ thread safe
	// configuration variables, once created, cannot be removed

	// structures Config* store a reference to the config variable and are very fast to access, but are _not_ thread safe
	// functions configSet* and configGet* are simpler to use for one-time access

	// assigning a value to config variable also changes type of the variable
	// reading a value from config variable converts the value from the type of the variable to the requested type, and does not change the type of the variable itself

	// ini values are mapped to config variables as 'prefix/section/item' = 'value'

	class Ini;

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

	namespace privat
	{
		struct ConfigVariable;
	}

	struct CAGE_CORE_API ConfigString
	{
		explicit ConfigString(const String &name);
		explicit ConfigString(const String &name, const String &default_);
		operator String() const;
		ConfigString &operator=(const String &value);

	private:
		privat::ConfigVariable *data = nullptr;
	};
	CAGE_CORE_API void configSetString(const String &name, const String &value);
	CAGE_CORE_API String configGetString(const String &name, const String &default_ = "");

#define GCHL_CONFIG(T, t) \
	struct CAGE_CORE_API Config##T \
	{ \
		explicit Config##T(const String &name); \
		explicit Config##T(const String &name, t default_); \
		operator t() const; \
		Config##T &operator=(t value); \
\
	private: \
		privat::ConfigVariable *data = nullptr; \
	}; \
	CAGE_CORE_API void configSet##T(const String &name, t value); \
	CAGE_CORE_API t configGet##T(const String &name, t default_ = 0);
	GCHL_CONFIG(Bool, bool)
	GCHL_CONFIG(Sint32, sint32)
	GCHL_CONFIG(Sint64, sint64)
	GCHL_CONFIG(Uint32, uint32)
	GCHL_CONFIG(Uint64, uint64)
	GCHL_CONFIG(Float, float)
	GCHL_CONFIG(Double, double)
#undef GCHL_CONFIG

	CAGE_CORE_API void configSetDynamic(const String &name, const String &value); // changes the type of the config variable to the one best suited for the value

	CAGE_CORE_API String configTypeToString(const ConfigTypeEnum type);
	CAGE_CORE_API ConfigTypeEnum configGetType(const String &name);

	CAGE_CORE_API void configApplyIni(const Ini *ini, const String &prefix);
	CAGE_CORE_API Holder<Ini> configGenerateIni(const String &prefix);
	CAGE_CORE_API void configImportIni(const String &filename, const String &prefix);
	CAGE_CORE_API void configExportIni(const String &filename, const String &prefix);

	class CAGE_CORE_API ConfigList : private Immovable
	{
	public:
		bool valid() const;
		String name() const;
		ConfigTypeEnum type() const;
		String typeName() const;
		bool getBool() const;
		sint32 getSint32() const;
		uint32 getUint32() const;
		sint64 getSint64() const;
		uint64 getUint64() const;
		float getFloat() const;
		double getDouble() const;
		String getString() const;
		void next();
	};

	CAGE_CORE_API Holder<ConfigList> newConfigList();

	namespace detail
	{
		CAGE_CORE_API String globalConfigPrefix();
	}
}

#endif // guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
