#include <cage-core/files.h>
#include <cage-core/config.h>
#include <cage-core/debug.h>

#include "database.h"

bool isNameDatabank(const String &name)
{
	return isPattern(name, "", "", ".assets");
}

bool isNameIgnored(const String &name)
{
	for (const String &it : configIgnoreExtensions)
	{
		if (isPattern(name, "", "", it))
			return true;
	}
	for (const String &it : configIgnorePaths)
	{
		if (isPattern(name, it, "", ""))
			return true;
	}
	return false;
}

namespace
{
	void findFiles(std::map<String, uint64> &files, const String &path)
	{
		const String pth = pathJoin(configPathInput, path);
		CAGE_LOG(SeverityEnum::Info, "database", Stringizer() + "checking path '" + pth + "'");
		Holder<DirectoryList> d = newDirectoryList(pth);
		while (d->valid())
		{
			const String p = pathJoin(path, d->name());
			if (!isNameIgnored(p))
			{
				if (d->isDirectory())
					findFiles(files, p);
				else
				{
					const uint64 lt = d->lastChange();
					files[p] = lt;
				}
			}
			d->next();
		}
	}
}

std::map<String, uint64> findFiles()
{
	std::map<String, uint64> files;
	findFiles(files, "");
	return files;
}

void checkOutputDir()
{
	const PathTypeFlags t = pathType(configPathOutput);
	if (configOutputArchive && any(t & PathTypeFlags::NotFound))
		return pathCreateArchive(configPathOutput);
	if (any(t & PathTypeFlags::Archive))
		return;
	// the output is not an archive, output to it directly
	configPathIntermediate = "";
}

void moveIntermediateFiles()
{
	Holder<DirectoryList> listIn = newDirectoryList(configPathIntermediate);
	Holder<DirectoryList> listOut = newDirectoryList(configPathOutput); // keep the archive open until all files are written (this significantly speeds up the moving process, but it causes the process to keep all the files in memory)
	uint64 movedSize = 0;
	while (listIn->valid())
	{
		if (movedSize > configArchiveWriteThreshold)
		{
			listOut.clear(); // close the archive
			listOut = newDirectoryList(configPathOutput); // reopen the archive
			movedSize = 0;
		}
		const String f = pathJoin(configPathIntermediate, listIn->name());
		const String t = pathJoin(configPathOutput, listIn->name());
		movedSize += readFile(f)->size();
		pathMove(f, t);
		listIn->next();
	}
	pathRemove(configPathIntermediate);
}
