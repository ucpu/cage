#ifndef guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
#define guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F

#include "core.h"

namespace cage
{
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
		string,
	};

	CAGE_CORE_API string configTypeToString(const ConfigTypeEnum type);
	CAGE_CORE_API void configSetDynamic(const string &name, const string &value);
	CAGE_CORE_API ConfigTypeEnum configGetType(const string &name);

#define GCHL_CONFIG(T, t) \
	struct CAGE_CORE_API CAGE_JOIN(Config, T) { CAGE_JOIN(Config, T)(const string &name); CAGE_JOIN(Config, T)(const string &name, t default_); operator t() const; CAGE_JOIN(Config, T) &operator = (t value); private: void *data; }; \
	CAGE_CORE_API void CAGE_JOIN(configSet, T)(const string &name, t value); \
	CAGE_CORE_API t CAGE_JOIN(configGet, T)(const string &name, t default_ = 0);
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
		ConfigString(const string &name);
		ConfigString(const string &name, const string &default_);
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

	CAGE_CORE_API void configApplyIni(const Ini *ini, const string &prefix);
	CAGE_CORE_API Holder<Ini> configGenerateIni(const string &prefix);
	CAGE_CORE_API void configLoadIni(const string &filename, const string &prefix);
	CAGE_CORE_API void configSaveIni(const string &filename, const string &prefix);

	namespace detail
	{
		CAGE_CORE_API string getConfigAppPrefix();
	}
}

#endif // guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
