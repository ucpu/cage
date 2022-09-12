#ifndef guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
#define guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_

#include <cage-core/string.h> // StringComparatorFast
#include <cage-core/serialization.h>
#include <cage-core/config.h>

#include <set>
#include <map>
#include <vector>

using namespace cage;

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

	friend Serializer &operator << (Serializer &ser, const SchemeField &s);
	friend Deserializer &operator >> (Deserializer &des, SchemeField &s);
};

struct Scheme
{
	String name;
	String processor;
	std::map<String, SchemeField> schemeFields;
	uint32 schemeIndex = m;

	void parse(Ini *ini);
	bool applyOnAsset(struct Asset &ass);

	friend Serializer &operator << (Serializer &ser, const Scheme &s);
	friend Deserializer &operator >> (Deserializer &des, Scheme &s);
};

extern std::map<String, Holder<Scheme>, StringComparatorFast> schemes;

void loadSchemes();

struct Asset
{
	String name;
	String aliasName;
	String scheme;
	String databank;
	std::map<String, String, StringComparatorFast> fields;
	std::set<String, StringComparatorFast> files;
	std::set<String, StringComparatorFast> references;
	bool corrupted = true;
	bool needNotify = false;

	uint32 outputPath() const;
	uint32 aliasPath() const;

	friend Serializer &operator << (Serializer &ser, const Asset &s);
	friend Deserializer &operator >> (Deserializer &des, Asset &s);
};

extern std::map<String, Holder<Asset>, StringComparatorFast> assets;

extern uint64 lastModificationTime;
extern std::set<String, StringComparatorFast> corruptedDatabanks;

bool databankParse(const String &path);
void databanksLoad();
void databanksSave();

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
extern std::set<String, StringComparatorFast> configIgnoreExtensions;
extern std::set<String, StringComparatorFast> configIgnorePaths;

void configParseCmd(int argc, const char *args[]);

void notifierInitialize();
void notifierAcceptConnections();
void notifierSendNotifications();

bool isNameDatabank(const String &name);
bool isNameIgnored(const String &name);
std::map<String, uint64, StringComparatorFast> findFiles();
void checkOutputDir();
void moveIntermediateFiles();

extern bool verdictValue;

void start();
void listen();

#endif // guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
