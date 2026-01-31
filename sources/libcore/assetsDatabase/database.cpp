#include "database.h"

#include <cage-core/hashString.h>
#include <cage-core/ini.h>
#include <cage-core/pointerRangeHolder.h>

namespace cage
{
	Holder<PointerRange<String>> DatabaseAsset::files() const
	{
		const DatabaseAssetImpl *impl = static_cast<const DatabaseAssetImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->files)
			result.push_back(it);
		return result;
	}

	Holder<PointerRange<String>> DatabaseAsset::references() const
	{
		const DatabaseAssetImpl *impl = static_cast<const DatabaseAssetImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->references)
			result.push_back(it);
		return result;
	}

	Holder<PointerRange<std::pair<String, String>>> DatabaseAsset::configuration() const
	{
		const DatabaseAssetImpl *impl = static_cast<const DatabaseAssetImpl *>(this);
		PointerRangeHolder<std::pair<String, String>> result;
		for (const auto &it : impl->fields)
			result.push_back(it);
		return result;
	}

	String DatabaseStatus::print() const
	{
		if (ok)
			return Stringizer() + "ok, assets: " + totalAssets + ", modified: " + modifiedAssets;
		return Stringizer() + "fail, corrupted definitions: " + corruptedDefinitions + ", corrupted assets: " + corruptedAssets + ", missing references: " + missingReferences;
	}

	Holder<PointerRange<String>> AssetsDatabase::allAssets() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->assets)
			result.push_back(it.first);
		return result;
	}

	Holder<PointerRange<String>> AssetsDatabase::assetsDependantOnFile(const String &file) const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->assets)
			if (it.second->files.count(file))
				result.push_back(it.first);
		return result;
	}

	Holder<PointerRange<std::pair<String, String>>> AssetsDatabase::assetsMissingReferences() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		PointerRangeHolder<std::pair<String, String>> result;
		for (const auto &it : impl->assets)
			for (const auto &ref : it.second->references)
				if (!impl->assets.count(ref) && !impl->injectedNames.count(HashString(ref)))
					result.push_back({ it.first, ref });
		return result;
	}

	Holder<PointerRange<String>> AssetsDatabase::corruptedAssets() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->assets)
			if (it.second->corrupted)
				result.push_back(it.first);
		return result;
	}

	Holder<PointerRange<String>> AssetsDatabase::modifiedAssets() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->assets)
			if (it.second->modified)
				result.push_back(it.first);
		return result;
	}

	const DatabaseAsset *AssetsDatabase::asset(const String &name) const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		const auto it = impl->assets.find(name);
		if (it == impl->assets.end())
		{
			CAGE_LOG_THROW(Stringizer() + "asset name: " + name);
			CAGE_THROW_ERROR(Exception, "asset not found");
		}
		return +it->second;
	}

	const DatabaseStatus &AssetsDatabase::status() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		return impl->status;
	}

	void AssetsDatabase::printIssues() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		if (!impl->corruptedDefinitions.empty())
		{
			for (const auto &it : impl->corruptedDefinitions)
				CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "corrupted definition: " + it);
		}
		{
			const auto list = corruptedAssets();
			if (!list.empty())
			{
				for (const auto &it : list)
					CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "corrupted asset: " + it);
			}
		}
		{
			const auto list = assetsMissingReferences();
			if (!list.empty())
			{
				for (const auto &it : list)
					CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "asset: " + it.first + ", missing reference: " + it.second);
			}
		}
	}

	void AssetsDatabase::assetConfiguration(const String &name, const String &key, const String &value)
	{
		// AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		// todo
		CAGE_THROW_CRITICAL(Exception, "assetConfiguration not implemented");
	}

	void AssetsDatabase::markCorrupted(const String &name)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		const auto it = impl->assets.find(name);
		if (it == impl->assets.end())
		{
			CAGE_LOG_THROW(Stringizer() + "asset name: " + name);
			CAGE_THROW_ERROR(Exception, "asset not found");
		}
		it->second->corrupted = true;
	}

	void AssetsDatabase::markAllCorrupted()
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		for (auto &it : impl->assets)
			it.second->corrupted = true;
	}

	void AssetsDatabase::markModified(const String &name, bool modified)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		const auto it = impl->assets.find(name);
		if (it == impl->assets.end())
		{
			CAGE_LOG_THROW(Stringizer() + "asset name: " + name);
			CAGE_THROW_ERROR(Exception, "asset not found");
		}
		it->second->modified = modified;
	}

	void AssetsDatabase::markAllModified(bool modified)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		for (auto &it : impl->assets)
			it.second->modified = modified;
	}

	void AssetsDatabase::convertAssets()
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		impl->checkOutput();
		impl->detectChanges();
		impl->processAssets();
		impl->saveDatabase();
		CAGE_LOG(SeverityEnum::Info, "database", impl->status.print());
		if (!impl->status.ok)
		{
			printIssues();
			CAGE_THROW_ERROR(Exception, "converting assets failed");
		}
	}

	Holder<PointerRange<String>> AssetsDatabase::allSchemes() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		PointerRangeHolder<String> result;
		for (const auto &it : impl->schemes)
			result.push_back(it.first);
		return result;
	}

	Holder<Ini> AssetsDatabase::scheme(const String &name) const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		const auto it = impl->schemes.find(name);
		if (it == impl->schemes.end())
		{
			CAGE_LOG_THROW(Stringizer() + "scheme name: " + name);
			CAGE_THROW_ERROR(Exception, "scheme not found");
		}
		// todo serialize to ini
		CAGE_THROW_CRITICAL(Exception, "scheme not implemented");
	}

	void AssetsDatabase::scheme(const String &name, const Ini *scheme)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		Holder<Scheme> sch = systemMemory().createHolder<Scheme>();
		sch->name = name;
		sch->parse(scheme);
		impl->schemes[name] = std::move(sch);
	}

	void AssetsDatabase::ignoreExtension(const String &extension, bool ignore)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		if (ignore)
			impl->ignoredExtensions.insert(extension);
		else
			impl->ignoredExtensions.erase(extension);
	}

	void AssetsDatabase::ignorePath(const String &path, bool ignore)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		if (ignore)
			impl->ignoredPaths.insert(path);
		else
			impl->ignoredPaths.erase(path);
	}

	bool AssetsDatabase::isIgnored(const String &name) const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		for (const String &it : impl->ignoredExtensions)
		{
			if (isPattern(name, "", "", it))
				return true;
		}
		for (const String &it : impl->ignoredPaths)
		{
			if (isPattern(name, it, "", ""))
				return true;
		}
		return false;
	}

	void AssetsDatabase::injectExternalAsset(uint32 id, bool inject)
	{
		AssetsDatabaseImpl *impl = static_cast<AssetsDatabaseImpl *>(this);
		if (inject)
			impl->injectedNames.insert(id);
		else
			impl->injectedNames.erase(id);
	}

	const AssetsDatabaseCreateConfig &AssetsDatabase::config() const
	{
		const AssetsDatabaseImpl *impl = static_cast<const AssetsDatabaseImpl *>(this);
		return impl->config;
	}

	Holder<AssetsDatabase> newAssetsDatabase(const AssetsDatabaseCreateConfig &config)
	{
		Holder<AssetsDatabaseImpl> impl = systemMemory().createHolder<AssetsDatabaseImpl>(config);
		impl->loadDatabase();
		return std::move(impl).cast<AssetsDatabase>();
	}

	namespace
	{
		void loadSchemes(AssetsDatabase *database, const String &dir)
		{
			const auto list = pathListDirectory(dir);
			for (const String &path : list)
			{
				if (pathIsDirectory(path))
				{
					loadSchemes(database, path);
					continue;
				}
				if (!isPattern(path, "", "", ".scheme"))
					continue;
				const String name = pathExtractFilenameNoExtension(path);
				Holder<Ini> ini = newIni();
				ini->importFile(path);
				database->scheme(name, +ini);
			}
		}
	}

	void databaseLoadSchemes(AssetsDatabase *database, const String &schemesPath)
	{
		loadSchemes(database, schemesPath);
	}

	void databaseInjectExternal(AssetsDatabase *database, const String &externalDatabasePath)
	{
		Holder<AssetsDatabase> ext = newAssetsDatabase({ .databasePath = externalDatabasePath });
		for (const String &name : ext->allAssets())
			database->injectExternalAsset(ext->asset(name)->id);
	}
}
