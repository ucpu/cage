#ifndef guard_asset_h_036fe307_7f7d_4620_83af_ab2cda0818b9_
#define guard_asset_h_036fe307_7f7d_4620_83af_ab2cda0818b9_

typedef std::set<string, stringComparatorFast> stringSet;
typedef std::map<string, string, stringComparatorFast> stringMap;

struct assetStruct
{
	assetStruct();
	string name;
	string internationalName;
	string scheme;
	string databank;
	stringMap fields;
	stringSet files;
	stringSet references;
	bool corrupted;
	bool needNotify;

	void load(fileHandle *fileName);
	void save(fileHandle *fileName) const;
	string outputPath() const;
	string internationalizedPath() const;
	bool operator < (const assetStruct &other) const
	{
		return stringCompareFast(name, other.name);
	}
};

#endif // guard_asset_h_036fe307_7f7d_4620_83af_ab2cda0818b9_
