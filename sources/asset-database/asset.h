#ifndef guard_asset_h_036fe307_7f7d_4620_83af_ab2cda0818b9_
#define guard_asset_h_036fe307_7f7d_4620_83af_ab2cda0818b9_

#include <set>
#include <map>

typedef std::set<string, stringComparatorFast> StringSet;
typedef std::map<string, string, stringComparatorFast> StringMap;

struct Asset
{
	Asset();
	string name;
	string aliasName;
	string scheme;
	string databank;
	StringMap fields;
	StringSet files;
	StringSet references;
	bool corrupted;
	bool needNotify;

	void load(File *fileName);
	void save(File *fileName) const;
	string outputPath() const;
	string aliasPath() const;
	bool operator < (const Asset &other) const
	{
		return stringComparatorFast()(name, other.name);
	}
};

#endif // guard_asset_h_036fe307_7f7d_4620_83af_ab2cda0818b9_
