#include <map>

#include "filesystem.h"
#include <cage-core/concurrent.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	archiveVirtual::archiveVirtual(const string &path) : myPath(path)
	{}

	void pathCreateArchive(const string &path, const string &options)
	{
		// someday, switch based on the options may be implemented here to create different types of archives
		// the options are passed on to allow for other options (compression, encryption, ...)
		archiveCreateZip(path, options);
	}

	void mixedMove(std::shared_ptr<archiveVirtual> &af, const string &pf, std::shared_ptr<archiveVirtual> &at, const string &pt)
	{
		{
			holder<fileClass> f = af ? af->file(pf, fileMode(true, false)) : realNewFile(pf, fileMode(true, false));
			holder<fileClass> t = at ? at->file(pt, fileMode(false, true)) : realNewFile(pt, fileMode(false, true));
			// todo split big files into multiple smaller steps
			memoryBuffer b = f->readBuffer(f->size());
			f->close();
			t->writeBuffer(b);
			t->close();
		}
		if (af)
			af->remove(pf);
		else
			realRemove(pf);
	}

	namespace
	{
		std::shared_ptr<archiveVirtual> archiveTryGet(const string &path)
		{
			static holder<mutexClass> *mutex = new holder<mutexClass>(newMutex()); // this leak is intentional
			static std::map<string, std::weak_ptr<archiveVirtual>> *map = new std::map<string, std::weak_ptr<archiveVirtual>>(); // this leak is intentional
			scopeLock<mutexClass> l(*mutex);
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
				detail::overrideException oe;
				auto a = archiveOpenZip(path);
				CAGE_ASSERT_RUNTIME(a);
				(*map)[path] = a;
				return a;
			}
			catch (const exception &)
			{
				return {};
			}
		}

		std::shared_ptr<archiveVirtual> recursiveFind(const string &path, bool matchExact, string &insidePath)
		{
			if (matchExact)
			{
				pathTypeFlags t = realType(path);
				if ((t & pathTypeFlags::File) == pathTypeFlags::File)
					return archiveTryGet(path);
				if ((t & pathTypeFlags::NotFound) == pathTypeFlags::None)
					return {}; // the path exists (most likely a directory) and is not an archive
			}
			if (pathExtractPathNoDrive(path) == "/")
				return {}; // do not throw an exception by going beyond root
			string a = pathJoin(path, "..");
			string b = path.subString(a.length() + 1, m);
			CAGE_ASSERT_RUNTIME(pathJoin(a, b) == path, a, b, path);
			insidePath = pathJoin(b, insidePath);
			return recursiveFind(a, true, insidePath);
		}
	}

	std::shared_ptr<archiveVirtual> archiveFindTowardsRoot(const string &path, bool matchExact, string &insidePath)
	{
		CAGE_ASSERT_RUNTIME(insidePath.empty());
		return recursiveFind(pathToAbs(path), matchExact, insidePath);
	}
}

