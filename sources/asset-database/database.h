#ifndef guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
#define guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_

#include <map>
#include <set>

#include <cage-core/config.h>
#include <cage-core/string.h> // StringComparatorFast

using namespace cage;

namespace cage
{
	struct Serializer;
	struct Deserializer;
}

struct SchemeField
{
	String name;
	String display;
	String hint;
	String type;
	String min;
	String max;
	String defaul;
	String values;

	bool valid() const;
	bool applyToAssetField(String &val, const String &assetName) const;

	friend Serializer &operator<<(Serializer &ser, const SchemeField &s);
	friend Deserializer &operator>>(Deserializer &des, SchemeField &s);
};

struct Scheme
{
	String name;
	String processor;
	std::map<String, SchemeField> schemeFields;
	uint32 schemeIndex = m;

	void parse(Ini *ini);
	bool applyOnAsset(struct Asset &ass);

	friend Serializer &operator<<(Serializer &ser, const Scheme &s);
	friend Deserializer &operator>>(Deserializer &des, Scheme &s);
};

struct Asset
{
	String name;
	String scheme;
	String databank;
	std::map<String, String, StringComparatorFast> fields;
	std::set<String, StringComparatorFast> files;
	std::set<String, StringComparatorFast> references;
	bool corrupted = true;
	bool needNotify = false;

	uint32 outputPath() const;

	friend Serializer &operator<<(Serializer &ser, const Asset &s);
	friend Deserializer &operator>>(Deserializer &des, Asset &s);
};

#endif // guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
