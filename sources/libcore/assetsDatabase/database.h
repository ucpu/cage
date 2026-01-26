#ifndef guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
#define guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_

#include <map>
#include <set>
#include <unordered_set>

#include <cage-core/assetsDatabase.h>
#include <cage-core/files.h>
#include <cage-core/string.h> // StringComparatorFast

namespace cage
{
	struct Serializer;
	struct Deserializer;
	class DatabaseAssetImpl;

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

		void parse(const Ini *ini);
		bool applyOnAsset(DatabaseAssetImpl &ass);

		friend Serializer &operator<<(Serializer &ser, const Scheme &s);
		friend Deserializer &operator>>(Deserializer &des, Scheme &s);
	};

	class DatabaseAssetImpl : public DatabaseAsset
	{
	public:
		std::map<String, String, StringComparatorFast> fields;
		std::set<String, StringComparatorFast> files;
		std::set<String, StringComparatorFast> references;

		friend Serializer &operator<<(Serializer &ser, const DatabaseAssetImpl &s);
		friend Deserializer &operator>>(Deserializer &des, DatabaseAssetImpl &s);
	};

	class AssetsDatabaseImpl : public AssetsDatabase
	{
	public:
		AssetsDatabaseImpl(const AssetsDatabaseCreateConfig &config);

		void loadDatabase();
		void saveDatabase();

		void updateStatus();
		void parseDefinition(const String &path);
		void detectChanges();

		void checkOutput();
		void processAssets();

		AssetsDatabaseCreateConfig config;
		DatabaseStatus status;

		std::map<String, Holder<Scheme>, StringComparatorFast> schemes;
		std::set<String, StringComparatorFast> ignoredExtensions;
		std::set<String, StringComparatorFast> ignoredPaths;
		std::unordered_set<uint32> injectedNames;

		std::map<String, Holder<DatabaseAssetImpl>, StringComparatorFast> assets;
		std::set<String, StringComparatorFast> corruptedDefinitions;
		PathLastChange lastModificationTime;
	};
}

#endif // guard_database_h_f17e7ce9_c9c5_49b3_b59d_c42929085c79_
