#include <cage-core/string.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include "files.h"

#include <map>

namespace cage
{
	FileAbstract::FileAbstract(const string &path, const FileMode &mode) : myPath(path), mode(mode)
	{
		CAGE_ASSERT(path == pathSimplify(path));
	}

	void FileAbstract::read(PointerRange<char> buffer)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "reading from write-only file");
	}

	void FileAbstract::write(PointerRange<const char> buffer)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "writing to read-only file");
	}

	ArchiveAbstract::ArchiveAbstract(const string & path) : myPath(path)
	{
		CAGE_ASSERT(path == pathSimplify(path));
	}

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
		struct ArchivesCache
		{
			Holder<Mutex> mutex = newMutex();
			std::map<string, std::weak_ptr<ArchiveAbstract>, StringComparatorFast> map;
		};

		ArchivesCache &archivesCache()
		{
			static ArchivesCache *cache = new ArchivesCache(); // this leak is intentional
			return *cache;
		}

		std::shared_ptr<ArchiveAbstract> archiveTryGet(const std::shared_ptr<ArchiveAbstract> &parent, const string &inPath)
		{
			const string fullPath = parent ? pathJoin(parent->myPath, inPath) : inPath;
			ArchivesCache &cache = archivesCache();
			auto it = cache.map.find(fullPath);
			if (it != cache.map.end())
			{
				auto a = it->second.lock();
				if (a)
					return a;
			}
			try
			{
				detail::OverrideException oe;
				auto a = archiveOpenZip(parent ? parent->openFile(inPath, FileMode(true, true)) : realNewFile(fullPath, FileMode(true, true)));
				CAGE_ASSERT(a);
				cache.map[fullPath] = a;
				return a;
			}
			catch (const Exception &)
			{
				return {};
			}
		}

		std::shared_ptr<ArchiveAbstract> archiveTryGet(const string &path)
		{
			return archiveTryGet({}, path);
		}

		void walkLeft(string &p, string &i)
		{
			const string s = pathJoin(p, "..");
			i = pathJoin(subString(p, s.length() + 1, m), i);
			p = s;
		}

		void walkRight(string &p, string &i)
		{
			const string k = split(i, "/");
			p = pathJoin(p, k);
		}

#ifdef CAGE_DEBUG
		class WalkTester
		{
		public:
			WalkTester()
			{
				{
					string l = "/abc/def/ghi";
					string r = "jkl/mno/pqr";
					walkLeft(l, r);
					CAGE_ASSERT(l == "/abc/def");
					CAGE_ASSERT(r == "ghi/jkl/mno/pqr");
				}
				{
					string l = "/abc/def/ghi";
					string r = "jkl/mno/pqr";
					walkRight(l, r);
					CAGE_ASSERT(l == "/abc/def/ghi/jkl");
					CAGE_ASSERT(r == "mno/pqr");
				}
			}
		} walkTesterInstance;
#endif // CAGE_DEBUG

		void walkRealPath(string &p, string &inside)
		{
			while (any(realType(p) & PathTypeFlags::NotFound))
				walkLeft(p, inside);
		}

		string walkImaginaryPath(std::shared_ptr<ArchiveAbstract> &a, bool allowExactMatch, const string &inside)
		{
			CAGE_ASSERT(a);
			string p, i = inside;
			while (!i.empty())
			{
				if (!allowExactMatch && inside.empty())
					break;
				walkRight(p, i);
				std::shared_ptr<ArchiveAbstract> b = archiveTryGet(a, p);
				if (b)
				{
					a = b;
					return walkImaginaryPath(a, allowExactMatch, i);
				}
			}
			return inside;
		}
	}

	std::shared_ptr<ArchiveAbstract> archiveFindTowardsRoot(const string &path_, bool allowExactMatch, string &inside)
	{
		CAGE_ASSERT(inside.empty());
		string path = pathToAbs(path_);
		ArchivesCache &cache = archivesCache();
		ScopeLock<Mutex> l(cache.mutex);
		CAGE_ASSERT(inside.empty());
		walkRealPath(path, inside);
		if (!allowExactMatch && inside.empty())
			return {}; // exact match is forbidden
		std::shared_ptr<ArchiveAbstract> a = archiveTryGet(path);
		if (!a)
			return {}; // does not exist
		inside = walkImaginaryPath(a, allowExactMatch, inside);
		return a;
	}
}
