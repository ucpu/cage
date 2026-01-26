#ifndef guard_assetsDatabase_h_wert45uipdf
#define guard_assetsDatabase_h_wert45uipdf

#include <cage-core/core.h>

namespace cage
{
	class Ini;

	class CAGE_CORE_API DatabaseAsset : private Immovable
	{
	public:
		String name;
		String scheme;
		String definitionPath; // the "*.assets" file where this asset was registered
		uint32 id = 0;
		bool corrupted = true; // the asset needs converting (and potentially also fixing sources)
		bool modified = false; // the asset was converted and should be announced to all listening games

		Holder<PointerRange<String>> files() const; // list of paths to files the asset depends on
		Holder<PointerRange<String>> references() const; // list of other assets used by this asset
		Holder<PointerRange<std::pair<String, String>>> configuration() const;
	};

	struct CAGE_CORE_API DatabaseStatus
	{
		//uint32 totalDefinitions = 0; // number of "*.assets" files
		uint32 corruptedDefinitions = 0;
		uint32 totalAssets = 0;
		uint32 corruptedAssets = 0;
		uint32 modifiedAssets = 0;
		uint32 missingReferences = 0;
		bool ok = false;

		String print() const;
	};

	struct AssetsDatabaseCreateConfig;

	class CAGE_CORE_API AssetsDatabase : private Immovable
	{
	public:
		// read

		Holder<PointerRange<String>> allAssets() const;
		Holder<PointerRange<String>> assetsDependantOnFile(const String &file) const;
		Holder<PointerRange<std::pair<String, String>>> assetsMissingReferences() const;
		Holder<PointerRange<String>> corruptedAssets() const;
		Holder<PointerRange<String>> modifiedAssets() const;
		const DatabaseAsset *asset(const String &name) const;
		const DatabaseStatus &status() const;
		void printIssues() const;

		// modification

		void assetConfiguration(const String &name, const String &key, const String &value);
		void markCorrupted(const String &name);
		void markAllCorrupted();
		void markModified(const String &name, bool modified);
		void markAllModified(bool modified);
		void convertAssets();

		// configuration

		Holder<PointerRange<String>> allSchemes() const;
		Holder<Ini> scheme(const String &name) const;
		void scheme(const String &name, const Ini *scheme); // nullptr to remove

		void ignoreExtension(const String &extension, bool ignore = true); // eg. ".blend1"
		void ignorePath(const String &path, bool ignore = true); // eg. ".git"
		bool isIgnored(const String &name) const;
		void injectExternalAsset(uint32 id, bool inject = true);

		const AssetsDatabaseCreateConfig &config() const;
	};

	struct CAGE_CORE_API AssetsDatabaseCreateConfig
	{
		String databasePath = "database";
		String inputPath = "data";
		String intermediatePath = "assetsTmp";
		String outputListByNames = "byName.txt";
		String outputListByIds = "byId.txt";
		String outputPath = "assets.carch";
		bool outputArchive = true;
	};

	CAGE_CORE_API Holder<AssetsDatabase> newAssetsDatabase(const AssetsDatabaseCreateConfig &config);

	CAGE_CORE_API void databaseLoadSchemes(AssetsDatabase *database, const String &schemesPath);
	CAGE_CORE_API void databaseInjectExternal(AssetsDatabase *database, const String &externalDatabasePath);
}

#endif // guard_assetsDatabase_h_wert45uipdf
