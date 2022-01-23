#include <cage-core/string.h>
#include <cage-core/stdHash.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include "files.h"

#include <robin_hood.h>

#include <vector>
#include <atomic>

namespace cage
{
	FileAbstract::FileAbstract(const String &path, const FileMode &mode) : myPath(path), myMode(mode)
	{}

	void FileAbstract::reopenForModification()
	{
		CAGE_THROW_CRITICAL(NotImplemented, "reopening abstract file");
	}

	void FileAbstract::readAt(PointerRange<char> buffer, uintPtr at)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "reading with offset from abstract file");
	}

	FileMode FileAbstract::mode() const
	{
		return myMode;
	}

	ArchiveAbstract::ArchiveAbstract(const String & path) : myPath(path)
	{
		CAGE_ASSERT(path == pathSimplify(path));
	}

	DirectoryListAbstract::DirectoryListAbstract(const String &path) : myPath(path)
	{}

	String DirectoryListAbstract::fullPath() const
	{
		return pathJoin(myPath, name());
	}

	void mixedMove(std::shared_ptr<ArchiveAbstract> &af, const String &pf, std::shared_ptr<ArchiveAbstract> &at, const String &pt)
	{
		{
			Holder<File> f = af->openFile(pf, FileMode(true, false));
			Holder<File> t = at->openFile(pt, FileMode(false, true));
			// todo split big files into multiple smaller steps
			Holder<PointerRange<char>> b = f->readAll();
			f->close();
			t->write(b);
			t->close();
		}
		af->remove(pf);
	}

	namespace
	{
		struct ArchiveWithExisting
		{
			std::shared_ptr<ArchiveAbstract> archive;
			bool existing = false;
		};

		struct ArchivesCache : private Immovable
		{
			ArchiveWithExisting getOrCreate(const String &path, Delegate<std::shared_ptr<ArchiveAbstract>(const String &)> ctor)
			{
				CAGE_ASSERT(path == pathSimplify(path));
				CAGE_ASSERT(path == pathToAbs(path));
				while (true)
				{
					{
						ScopeLock lock(mutex);
						auto it = map.find(path);
						if (it == map.end())
						{
							auto a = ctor(path); // this may call _erase_
							if (!a)
								return {};
							map[path] = a;
							a->succesfullyMounted = true; // enable the lock inside _erase_
							return { a, false };
						}
						else
						{
							auto a = it->second.lock();
							if (a)
								return { a, true };
						}
					}
					// the archives destructor is still running
					// release the lock so that the _erase_ can get it and try again later
					threadYield();
				}
			}

			void erase(ArchiveAbstract *a)
			{
				if (!a->succesfullyMounted)
					return;
				ScopeLock lock(mutex);
				CAGE_ASSERT(map.count(a->myPath));
				CAGE_ASSERT(!map[a->myPath].lock());
				map.erase(a->myPath);
			}

		private:
			robin_hood::unordered_map<String, std::weak_ptr<ArchiveAbstract>> map;
			Holder<Mutex> mutex = newMutex(); // access to map must be serialized because the _erase_ method can be called on any thread and outside the archiveFindTowardsRootMutex
		};

		ArchivesCache &archivesCache()
		{
			static ArchivesCache *cache = new ArchivesCache(); // this leak is intentional
			return *cache;
		}

		std::shared_ptr<ArchiveAbstract> archiveCtorReal(const String &path)
		{
			return archiveOpenReal(path);
		}

		std::shared_ptr<ArchiveAbstract> archiveCtorArchive(ArchiveAbstract *parent, const String &fullPath)
		{
			CAGE_ASSERT(parent);
			const String inPath = pathToRel(fullPath, parent->myPath);
			return archiveOpenZipTry(parent->openFile(inPath, FileMode(true, false)));
		}

		void walkLeft(String &p, String &i)
		{
			const String s = pathJoin(p, "..");
			i = pathJoin(subString(p, s.length() + 1, m), i);
			p = s;
		}

		void walkRight(String &p, String &i)
		{
			const String k = split(i, "/");
			p = pathJoin(p, k);
		}

#ifdef CAGE_DEBUG
		class WalkTester : private Immovable
		{
		public:
			WalkTester()
			{
				{
					String l = "/abc/def/ghi";
					String r = "jkl/mno/pqr";
					walkLeft(l, r);
					CAGE_ASSERT(l == "/abc/def");
					CAGE_ASSERT(r == "ghi/jkl/mno/pqr");
				}
				{
					String l = "/abc/def/ghi";
					String r = "jkl/mno/pqr";
					walkRight(l, r);
					CAGE_ASSERT(l == "/abc/def/ghi/jkl");
					CAGE_ASSERT(r == "mno/pqr");
				}
			}
		} walkTesterInstance;
#endif // CAGE_DEBUG

		ArchiveWithPath archiveFindIterate(std::shared_ptr<ArchiveAbstract> parent, const String &path, ArchiveFindModeEnum mode)
		{
			CAGE_ASSERT(parent);
			String p, i = path;
			while (true)
			{
				if (i.empty())
					return { parent, path };
				walkRight(p, i);
				if (any(parent->type(p) & PathTypeFlags::File))
				{
					const String fp = pathJoin(parent->myPath, p);
					ArchiveWithExisting b = archivesCache().getOrCreate(fp, Delegate<std::shared_ptr<ArchiveAbstract>(const String &)>().bind<ArchiveAbstract *, &archiveCtorArchive>(parent.get()));
					if (b.archive)
					{
						if (i.empty())
						{
							switch (mode)
							{
							case ArchiveFindModeEnum::FileExclusiveThrow:
								if (b.existing)
								{
									CAGE_LOG_THROW(Stringizer() + "path: '" + pathJoin(parent->myPath, path) + "'");
									CAGE_THROW_ERROR(Exception, "file cannot be manipulated, it is opened as archive");
								}
								return { parent, path };
							case ArchiveFindModeEnum::ArchiveExclusiveThrow:
								if (b.existing)
								{
									CAGE_LOG_THROW(Stringizer() + "path: '" + pathJoin(parent->myPath, path) + "'");
									CAGE_THROW_ERROR(Exception, "file cannot be manipulated, it is opened as archive");
								}
								return { b.archive };
							case ArchiveFindModeEnum::ArchiveExclusiveNull:
								if (b.existing)
									return {};
								return { b.archive };
							case ArchiveFindModeEnum::ArchiveShared:
								return { b.archive };
							}
						}
						return archiveFindIterate(b.archive, i, mode);
					}
				}
			}
		}

		// archiveFindTowardsRoot must be serialized to prevent situations, where one thread creates an archive while other thread tries to create a directory with the same name
		Mutex *archiveFindTowardsRootMutex()
		{
			static Holder<Mutex> *mut = new Holder<Mutex>(newMutex()); // intentional leak
			return +*mut;
		}
	}

	ArchiveWithPath archiveFindTowardsRoot(const String &path_, ArchiveFindModeEnum mode)
	{
		if (!pathIsValid(path_))
		{
			CAGE_LOG_THROW(Stringizer() + "path: '" + path_ + "'");
			CAGE_THROW_ERROR(Exception, "invalid path");
		}

		const String path = pathToAbs(path_);
		CAGE_ASSERT(pathSimplify(path) == path);

		String rootPath, inside;
		{
#ifdef CAGE_SYSTEM_WINDOWS
			inside = path;
			rootPath = split(inside, ":") + ":/";
			if (!inside.empty() && inside[0] == '/')
				inside = subString(inside, 1, m);
#else
			inside = subString(path, 1, m);
			rootPath = "/";
#endif // CAGE_SYSTEM_WINDOWS
		}
		CAGE_ASSERT(pathIsAbs(rootPath));
		CAGE_ASSERT(pathJoin(rootPath, inside) == path);

		ScopeLock lock(archiveFindTowardsRootMutex());

		std::shared_ptr<ArchiveAbstract> root = archivesCache().getOrCreate(rootPath, Delegate<std::shared_ptr<ArchiveAbstract>(const String &)>().bind<&archiveCtorReal>()).archive;

		ArchiveWithPath r = archiveFindIterate(root, inside, mode);
		switch (mode)
		{
		case ArchiveFindModeEnum::FileExclusiveThrow:
			CAGE_ASSERT(r.archive);
			CAGE_ASSERT(!r.path.empty());
			break;
		case ArchiveFindModeEnum::ArchiveExclusiveThrow:
			CAGE_ASSERT(r.archive);
			break;
		case ArchiveFindModeEnum::ArchiveExclusiveNull:
			break;
		case ArchiveFindModeEnum::ArchiveShared:
			CAGE_ASSERT(r.archive);
			break;
		}

		return r;
	}

	ArchiveAbstract::~ArchiveAbstract()
	{
		archivesCache().erase(this);
	}
}
