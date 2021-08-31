#ifndef guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
#define guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_

#include <cage-core/files.h>
#include <cage-core/serialization.h>
#include <cage-core/config.h>
#include <cage-core/string.h>

#include <set>
#include <map>

using namespace cage;

typedef std::set<String, StringComparatorFast> StringSet;
typedef std::map<String, String, StringComparatorFast> StringMap;

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

	typedef std::set<Holder<T>, ComparatorStruct> Type;
	typedef typename Type::iterator Iterator;

	Iterator find(const String &name) const
	{
		T tmp;
		tmp.name = name;
		Holder<T> tmh(&tmp, nullptr);
		return const_cast<HolderSet*>(this)->data.find(tmh);
	}

	T *retrieve(const String &name)
	{
		Iterator it = find(name);
		if (it == end())
			return nullptr;
		return const_cast<T*>(it->get());
	}

	T *insert(T &&value)
	{
		return const_cast<T*>(data.insert(systemMemory().createHolder<T>(std::move(value))).first->get());
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

	void load(File *f)
	{
		CAGE_ASSERT(size() == 0);
		uint32 s = 0;
		f->read(bufferView<char>(s));
		for (uint32 i = 0; i < s; i++)
		{
			T tmp;
			tmp.load(f);
			insert(std::move(tmp));
		}
	}

	void save(File *f)
	{
		uint32 s = size();
		f->write(bufferView(s));
		for (auto &it : data)
			it->save(f);
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
	inline bool operator < (const SchemeField &other) const
	{
		return StringComparatorFast()(name, other.name);
	}
};

struct Scheme
{
	String name;
	String processor;
	uint32 schemeIndex;
	HolderSet<SchemeField> schemeFields;

	void parse(Ini *ini);
	void load(File *file);
	void save(File *file);
	bool applyOnAsset(struct Asset &ass);
	inline bool operator < (const Scheme &other) const
	{
		return StringComparatorFast()(name, other.name);
	}
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

	void load(File *file);
	void save(File *file) const;
	String outputPath() const;
	String aliasPath() const;
	bool operator < (const Asset &other) const
	{
		return StringComparatorFast()(name, other.name);
	}
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

inline void read(File *f, String &s)
{
	uint32 l = 0;
	f->read(bufferView<char>(l));
	if (l >= String::MaxLength)
		CAGE_THROW_ERROR(Exception, "string too long");
	char buffer[String::MaxLength];
	f->read({ buffer, buffer + l });
	s = String({ buffer, buffer + l });
}

inline void write(File *f, const String &s)
{
	uint32 l = s.length();
	f->write(bufferView(l));
	f->write({ s.c_str(), s.c_str() + l });
}

#endif // guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
