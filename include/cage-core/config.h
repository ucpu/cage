#ifndef guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
#define guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F

namespace cage
{
	enum class configTypeEnum
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

	CAGE_API string configTypeToString(const configTypeEnum type);
	CAGE_API void configSetDynamic(const string &name, const string &value);
	CAGE_API configTypeEnum configGetType(const string &name);

#define GCHL_CONFIG(T, t) \
	struct CAGE_API CAGE_JOIN(config, T) { CAGE_JOIN(config, T)(const string &name); CAGE_JOIN(config, T)(const string &name, t default_); operator t() const; CAGE_JOIN(config, T) &operator = (t value); private: void *data; }; \
	CAGE_API void CAGE_JOIN(configSet, T)(const string &name, t value); \
	CAGE_API t CAGE_JOIN(configGet, T)(const string &name, t default_ = 0);
	GCHL_CONFIG(Bool, bool)
	GCHL_CONFIG(Sint32, sint32)
	GCHL_CONFIG(Sint64, sint64)
	GCHL_CONFIG(Uint32, uint32)
	GCHL_CONFIG(Uint64, uint64)
	GCHL_CONFIG(Float, float)
	GCHL_CONFIG(Double, double)
#undef GCHL_CONFIG

	struct CAGE_API configString
	{
		configString(const string &name);
		configString(const string &name, const string &default_);
		operator string() const;
		configString &operator = (const string &value);
	private:
		void *data;
	};
	CAGE_API void configSetString(const string &name, const string &value);
	CAGE_API string configGetString(const string &name, const string &default_ = "");

	class CAGE_API configListClass
	{
	public:
		bool valid() const;
		string name() const;
		configTypeEnum type() const;
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

	CAGE_API holder<configListClass> newConfigList();

	CAGE_API void configLoadIni(const string &filename, const string &prefix);
	CAGE_API void configSaveIni(const string &filename, const string &prefix);
}

#endif // guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
