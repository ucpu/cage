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

	void FileAbstract::reopenForModification()
	{
		CAGE_THROW_CRITICAL(NotImplemented, "reopening for modification a file that does not support it");
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
			std::shared_ptr<ArchiveAbstract> tryGet(const string &path) const
			{
				ScopeLock<Mutex> l(mutex);
				return tryGetNoLock(path);
			}

			std::shared_ptr<ArchiveAbstract> assign(std::shared_ptr<ArchiveAbstract> a)
			{
				// multiple threads might have opened the same archive simultaneously and the first one might already have some changes applied to it,
				// therefore we need to first check if another archive with the same path already exists
				// the newly opened archive can be safely discarded (outside lock) again as no changes could possibly be made to it yet
				ScopeLock<Mutex> l(mutex);
				auto old = tryGetNoLock(a->myPath);
				if (old)
					return old;
				map[a->myPath] = a;
				return a;
			}

		private:
			Holder<Mutex> mutex = newMutex();
			std::map<string, std::weak_ptr<ArchiveAbstract>, StringComparatorFast> map;

			std::shared_ptr<ArchiveAbstract> tryGetNoLock(const string &path) const
			{
				auto it = map.find(path);
				if (it != map.end())
				{
					auto l = it->second.lock();
					if (l)
					{
						CAGE_ASSERT(l->myPath == path);
					}
					return l;
				}
				return {};
			}
		};

		ArchivesCache &archivesCache()
		{
			static ArchivesCache *cache = new ArchivesCache(); // this leak is intentional
			return *cache;
		}

		std::shared_ptr<ArchiveAbstract> archiveTryGet(const std::shared_ptr<ArchiveAbstract> &parent, const string &inPath)
		{
			const string fullPath = parent ? pathJoin(parent->myPath, inPath) : inPath;
			CAGE_ASSERT(fullPath == pathSimplify(fullPath));
			ArchivesCache &cache = archivesCache();
			{
				auto a = cache.tryGet(fullPath);
				if (a)
					return a;
			}
			try
			{
				std::shared_ptr<ArchiveAbstract> a;
				{
					detail::OverrideException oe;
					a = archiveOpenZip(parent ? parent->openFile(inPath, FileMode(true, false)) : realNewFile(fullPath, FileMode(true, false)));
				}
				CAGE_ASSERT(a);
				return cache.assign(a);
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

		ArchiveInPath walkImaginaryPath(std::shared_ptr<ArchiveAbstract> &a, const string &path, bool allowExactMatch, bool archiveInsideArchive)
		{
			CAGE_ASSERT(a);
			string p, i = path;
			while (true)
			{
				if (i.empty())
					return { a, path, archiveInsideArchive };
				walkRight(p, i);
				std::shared_ptr<ArchiveAbstract> b = archiveTryGet(a, p);
				if (b)
					return walkImaginaryPath(b, i, allowExactMatch, true);
			}
		}
	}

	ArchiveInPath archiveFindTowardsRoot(const string &path_, bool allowExactMatch)
	{
		string path = pathToAbs(path_);
		string inside;
		walkRealPath(path, inside);
		if (!allowExactMatch && inside.empty())
			return {}; // exact match is forbidden
		std::shared_ptr<ArchiveAbstract> a = archiveTryGet(path);
		if (!a)
			return {}; // does not exist
		return walkImaginaryPath(a, inside, allowExactMatch, false);
	}
}
