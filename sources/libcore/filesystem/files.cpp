#include <cage-core/string.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include "files.h"

#include <map>

namespace cage
{
	FileAbstract::FileAbstract(const string &path, const FileMode &mode) : myPath(path), mode(mode)
	{}

	void FileAbstract::read(void *data, uintPtr size)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "reading from write-only file");
	}

	void FileAbstract::write(const void *data, uintPtr size)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "writing to read-only file");
	}

	ArchiveAbstract::ArchiveAbstract(const string & path) : myPath(path)
	{}

	DirectoryListAbstract::DirectoryListAbstract(const string &path) : myPath(path)
	{}

	string DirectoryListAbstract::fullPath() const
	{
		return pathJoin(myPath, name());
	}

	void mixedMove(std::shared_ptr<ArchiveAbstract> &af, const string &pf, std::shared_ptr<ArchiveAbstract> &at, const string &pt)
	{
		{
			Holder<File> f = af ? af->openFile(pf, FileMode(true, false)) : realNewFile(pf, FileMode(true, false));
			Holder<File> t = at ? at->openFile(pt, FileMode(false, true)) : realNewFile(pt, FileMode(false, true));
			// todo split big files into multiple smaller steps
			const MemoryBuffer b = f->readAll();
			f->close();
			t->write(b);
			t->close();
		}
		if (af)
			af->remove(pf);
		else
			realRemove(pf);
	}

	namespace
	{
		std::shared_ptr<ArchiveAbstract> archiveTryGet(const string &path)
		{
			static Holder<Mutex> *mutex = new Holder<Mutex>(newMutex()); // this leak is intentional
			static std::map<string, std::weak_ptr<ArchiveAbstract>> *map = new std::map<string, std::weak_ptr<ArchiveAbstract>>(); // this leak is intentional
			ScopeLock<Mutex> l(*mutex);
			auto it = map->find(path);
			if (it != map->end())
			{
				auto a = it->second.lock();
				if (a)
					return a;
			}
			// todo determine archive type of the file
			try
			{
				detail::OverrideException oe;
				auto a = archiveOpenZip(path);
				CAGE_ASSERT(a);
				(*map)[path] = a;
				return a;
			}
			catch (const Exception &)
			{
				return {};
			}
		}

		std::shared_ptr<ArchiveAbstract> recursiveFind(const string &path, bool matchExact, string &insidePath)
		{
			CAGE_ASSERT(path == pathSimplify(path));
			if (matchExact)
			{
				PathTypeFlags t = realType(path);
				if (any(t & PathTypeFlags::File))
					return archiveTryGet(path);
				if (none(t & PathTypeFlags::NotFound))
					return {}; // the path exists (most likely a directory) and is not an archive
			}
			if (pathExtractPathNoDrive(path) == "/")
				return {}; // do not throw an exception by going beyond root
			const string a = pathJoin(path, "..");
			const string b = subString(path, a.length() + 1, m);
			CAGE_ASSERT(pathJoin(a, b) == path);
			insidePath = pathJoin(b, insidePath);
			return recursiveFind(a, true, insidePath);
		}
	}

	std::shared_ptr<ArchiveAbstract> archiveFindTowardsRoot(const string &path, bool matchExact, string &insidePath)
	{
		CAGE_ASSERT(insidePath.empty());
		return recursiveFind(pathToAbs(path), matchExact, insidePath);
	}
}
