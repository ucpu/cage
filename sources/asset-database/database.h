#ifndef guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
#define guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_

#include <cage-core/string.h> // StringComparatorFast
#include <cage-core/serialization.h>

#include <set>
#include <map>

using namespace cage;

using StringSet = std::set<String, StringComparatorFast>;
using StringMap = std::map<String, String, StringComparatorFast>;

template<class T>
struct HolderSet
{
	struct ComparatorStruct
	{
		bool operator() (const Holder<T> &a, const Holder<T> &b) const
		{
			return *a < *b;
		}
	};

	using Type = std::set<Holder<T>, ComparatorStruct>;
	using Iterator = typename Type::iterator;

	Iterator find(const String &name) const
	{
		T tmp;
		tmp.name = name;
		Holder<T> tmh(&tmp, nullptr);
		return const_cast<HolderSet *>(this)->data.find(tmh);
	}

	T *retrieve(const String &name)
	{
		Iterator it = find(name);
		if (it == end())
			return nullptr;
		return const_cast<T *>(it->get());
	}

	T *insert(T &&value)
	{
		return const_cast<T *>(data.insert(systemMemory().createHolder<T>(std::move(value))).first->get());
	}

	Iterator erase(const Iterator &what)
	{
		return data.erase(what);
	}

	Iterator erase(const String &name)
	{
		return erase(find(name));
	}

	void clear()
	{
		data.clear();
	}

	Iterator begin()
	{
		return data.begin();
	}

	Iterator end()
	{
		return data.end();
	}

	bool exists(const String &name) const
	{
		return find(name) != data.end();
	}

	uint32 size() const
	{
		return numeric_cast<uint32>(data.size());
	}

	friend Serializer &operator << (Serializer &ser, const HolderSet &s)
	{
		ser << s.size();
		for (const auto &it : s.data)
			ser << *it;
		return ser;
	}

	friend Deserializer &operator >> (Deserializer &des, HolderSet &s)
	{
		CAGE_ASSERT(s.size() == 0);
		uint32 cnt = 0;
		des >> cnt;
		for (uint32 i = 0; i < cnt; i++)
		{
			T tmp;
			des >> tmp;
			s.insert(std::move(tmp));
		}
		return des;
	}

private:
	Type data;
};

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

	bool operator < (const SchemeField &other) const
	{
		return StringComparatorFast()(name, other.name);
	}

	friend Serializer &operator << (Serializer &ser, const SchemeField &s);
	friend Deserializer &operator >> (Deserializer &des, SchemeField &s);
};

struct Scheme
{
	String name;
	String processor;
	uint32 schemeIndex;
	HolderSet<SchemeField> schemeFields;

	void parse(Ini *ini);
	bool applyOnAsset(struct Asset &ass);

	bool operator < (const Scheme &other) const
	{
		return StringComparatorFast()(name, other.name);
	}

	friend Serializer &operator << (Serializer &ser, const Scheme &s);
	friend Deserializer &operator >> (Deserializer &des, Scheme &s);
};

struct Asset
{
	String name;
	String aliasName;
	String scheme;
	String databank;
	StringMap fields;
	StringSet files;
	StringSet references;
	bool corrupted = true;
	bool needNotify = false;

	String outputPath() const;
	String aliasPath() const;

	bool operator < (const Asset &other) const
	{
		return StringComparatorFast()(name, other.name);
	}

	friend Serializer &operator << (Serializer &ser, const Asset &s);
	friend Deserializer &operator >> (Deserializer &des, Asset &s);
};

extern ConfigString configPathInput;
extern ConfigString configPathOutput;
extern ConfigString configPathIntermediate;
extern ConfigString configPathDatabase;
extern ConfigString configPathByHash;
extern ConfigString configPathByName;
extern ConfigString configPathSchemes;
extern ConfigSint32 configNotifierPort;
extern ConfigUint64 configArchiveWriteThreshold;
extern ConfigBool configFromScratch;
extern ConfigBool configListening;
extern ConfigBool configOutputArchive;
extern StringSet configIgnoreExtensions;
extern StringSet configIgnorePaths;

void configParseCmd(int argc, const char *args[]);

void notifierInitialize(const uint16 port);
void notifierAccept();
void notifierNotify(const String &str);

void start();
void listen();
bool verdict();

#endif // guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
