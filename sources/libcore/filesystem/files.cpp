#include <cage-core/string.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/concurrent.h>
#include <cage-core/debug.h>

#include "files.h"

#include <map>
#include <vector>
#include <atomic>

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

	void FileAbstract::readAt(PointerRange<char> buffer, uintPtr at)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "reading from file at offset not supported");
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
			Holder<File> f = af->openFile(pf, FileMode(true, false));
			Holder<File> t = at->openFile(pt, FileMode(false, true));
			// todo split big files into multiple smaller steps
			const MemoryBuffer b = f->readAll();
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
			ArchiveWithExisting getOrCreate(const string &path, Delegate<std::shared_ptr<ArchiveAbstract>(const string &)> ctor)
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
			std::map<string, std::weak_ptr<ArchiveAbstract>, StringComparatorFast> map;
			Holder<Mutex> mutex = newMutex(); // access to map must be serialized because the _erase_ method can be called on any thread and outside the archiveFindTowardsRootMutex
		};

		ArchivesCache &archivesCache()
		{
			static ArchivesCache *cache = new ArchivesCache(); // this leak is intentional
			return *cache;
		}

		std::shared_ptr<ArchiveAbstract> archiveCtorReal(const string &path)
		{
			return archiveOpenReal(path);
		}

		std::shared_ptr<ArchiveAbstract> archiveCtorArchive(ArchiveAbstract *parent, const string &fullPath)
		{
			CAGE_ASSERT(parent);
			const string inPath = pathToRel(fullPath, parent->myPath);
			try
			{
				detail::OverrideException oe;
				return archiveOpenZip(parent->openFile(inPath, FileMode(true, false)));
			}
			catch (const Exception &)
			{
				return {};
			}
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
		class WalkTester : private Immovable
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

		ArchiveWithPath archiveFindIterate(std::shared_ptr<ArchiveAbstract> parent, const string &path, ArchiveFindModeEnum mode)
		{
			CAGE_ASSERT(parent);
			string p, i = path;
			while (true)
			{
				if (i.empty())
					return { parent, path };
				walkRight(p, i);
				if (any(parent->type(p) & PathTypeFlags::File))
				{
					const string fp = pathJoin(parent->myPath, p);
					ArchiveWithExisting b = archivesCache().getOrCreate(fp, Delegate<std::shared_ptr<ArchiveAbstract>(const string &)>().bind<ArchiveAbstract *, &archiveCtorArchive>(parent.get()));
					if (b.archive)
					{
						if (i.empty())
						{
							switch (mode)
							{
							case ArchiveFindModeEnum::FileExclusiveThrow:
								if (b.existing)
								{
									CAGE_LOG_THROW(stringizer() + "path: '" + pathJoin(parent->myPath, path) + "'");
									CAGE_THROW_ERROR(Exception, "file cannot be manipulated, it is opened as archive");
								}
								return { parent, path };
							case ArchiveFindModeEnum::ArchiveExclusiveThrow:
								if (b.existing)
								{
									CAGE_LOG_THROW(stringizer() + "path: '" + pathJoin(parent->myPath, path) + "'");
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

	ArchiveWithPath archiveFindTowardsRoot(const string &path_, ArchiveFindModeEnum mode)
	{
		if (!pathIsValid(path_))
		{
			CAGE_LOG_THROW(stringizer() + "path: '" + path_ + "'");
			CAGE_THROW_ERROR(Exception, "invalid path");
		}

		const string path = pathToAbs(path_);
		CAGE_ASSERT(pathSimplify(path) == path);

		string rootPath, inside;
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

		std::shared_ptr<ArchiveAbstract> root = archivesCache().getOrCreate(rootPath, Delegate<std::shared_ptr<ArchiveAbstract>(const string &)>().bind<&archiveCtorReal>()).archive;

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
