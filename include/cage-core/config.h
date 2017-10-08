#ifndef guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F
#define guard_config_h_B892887C89784EDE9E816F4BCF6EFB0F

namespace cage
{
	CAGE_API void configSetBool(const string &name, bool value);
	CAGE_API void configSetSint32(const string &name, sint32 value);
	CAGE_API void configSetUint32(const string &name, uint32 value);
	CAGE_API void configSetSint64(const string &name, sint64 value);
	CAGE_API void configSetUint64(const string &name, uint64 value);
	CAGE_API void configSetFloat(const string &name, float value);
	CAGE_API void configSetDouble(const string &name, double value);
	CAGE_API void configSetString(const string &name, const string &value);
	CAGE_API void configSetDynamic(const string &name, const string &value);

	CAGE_API bool configGetBool(const string &name, bool value = false);
	CAGE_API sint32 configGetSint32(const string &name, sint32 value = 0);
	CAGE_API uint32 configGetUint32(const string &name, uint32 value = 0);
	CAGE_API sint64 configGetSint64(const string &name, sint64 value = 0);
	CAGE_API uint64 configGetUint64(const string &name, uint64 value = 0);
	CAGE_API float configGetFloat(const string &name, float value = 0);
	CAGE_API double configGetDouble(const string &name, double value = 0);
	CAGE_API string configGetString(const string &name, const string &value = "");

	struct CAGE_API configBool
	{
		configBool(const string &name, bool value = false);
		operator bool() const;
		configBool &operator = (bool value);
	private:
		void *data;
	};

	struct CAGE_API configSint32
	{
		configSint32(const string &name, sint32 value = 0);
		operator sint32() const;
		configSint32 &operator = (sint32 value);
	private:
		void *data;
	};

	struct CAGE_API configUint32
	{
		configUint32(const string &name, uint32 value = 0);
		operator uint32() const;
		configUint32 &operator = (uint32 value);
	private:
		void *data;
	};

	struct CAGE_API configSint64
	{
		configSint64(const string &name, sint64 value = 0);
		operator sint64() const;
		configSint64 &operator = (sint64 value);
	private:
		void *data;
	};

	struct CAGE_API configUint64
	{
		configUint64(const string &name, uint64 value = 0);
		operator uint64() const;
		configUint64 &operator = (uint64 value);
	private:
		void *data;
	};

	struct CAGE_API configFloat
	{
		configFloat(const string &name, float value = 0);
		operator float() const;
		configFloat &operator = (float value);
	private:
		void *data;
	};

	struct CAGE_API configDouble
	{
		configDouble(const string &name, double value = 0);
		operator double() const;
		configDouble &operator = (double value);
	private:
		void *data;
	};

	struct CAGE_API configString
	{
		configString(const string &name, const string &value = "");
		operator string() const;
		configString &operator = (const string &value);
	private:
		void *data;
	};

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
