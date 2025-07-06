#include <atomic>
#include <map>
#include <vector>

#include "files.h"

#include <cage-core/debug.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/stdHash.h>
#include <cage-core/string.h>

namespace cage
{
	std::shared_ptr<ArchiveAbstract> archiveOpenZipTry(Holder<File> f);
	std::shared_ptr<ArchiveAbstract> archiveOpenCarchTry(Holder<File> f);
	std::shared_ptr<ArchiveAbstract> archiveOpenReal(const String &path);

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
				auto it = map.find(path);
				if (it != map.end())
				{
					std::shared_ptr<ArchiveAbstract> a = it->second.lock();
					if (a)
						return { a, true };
					map.erase(path);
				}
				std::shared_ptr<ArchiveAbstract> a = ctor(path); // this may call _erase_
				if (!a)
					return {};
				map[path] = a;
				return { a, false };
			}

			void erase(ArchiveAbstract *a)
			{
				auto it = map.find(a->myPath);
				if (it == map.end())
					return; // already erased
				auto b = it->second.lock();
				if (b && b.get() != a)
					return; // the stored archive is different
				map.erase(a->myPath);
			}

		private:
			std::unordered_map<String, std::weak_ptr<ArchiveAbstract>> map;
		};

		ArchivesCache &archivesCache()
		{
			static ArchivesCache *cache = new ArchivesCache(); // this leak is intentional
			return *cache;
		}

		std::shared_ptr<ArchiveAbstract> archiveCtorArchive(ArchiveAbstract *parent, const String &fullPath)
		{
			CAGE_ASSERT(parent);
			const String inPath = pathToRel(fullPath, parent->myPath);
			Holder<File> f = parent->openFile(inPath, FileMode(true, false));
			if (auto cap = archiveOpenCarchTry(f.share()))
				return cap;
			return archiveOpenZipTry(f.share());
		}

		void walkRight(String &p, String &i)
		{
			const String k = split(i, "/");
			p = pathJoin(p, k);
		}

#ifdef CAGE_DEBUG
		void walkLeft(String &p, String &i)
		{
			const String s = pathJoin(p, "..");
			i = pathJoin(subString(p, s.length() + 1, m), i);
			p = s;
		}

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
					const auto ctor = Delegate<std::shared_ptr<ArchiveAbstract>(const String &)>().bind<ArchiveAbstract *, &archiveCtorArchive>(parent.get());
					ArchiveWithExisting b = archivesCache().getOrCreate(fp, ctor);
					if (b.archive)
					{
						if (i.empty())
						{
							switch (mode)
							{
								case ArchiveFindModeEnum::FileExclusive:
									if (b.existing)
									{
										CAGE_LOG_THROW(Stringizer() + "path: " + pathJoin(parent->myPath, path));
										CAGE_THROW_ERROR(Exception, "file cannot be manipulated, it is opened as archive");
									}
									return { parent, path };
								case ArchiveFindModeEnum::ArchiveExclusive:
									if (b.existing)
									{
										CAGE_LOG_THROW(Stringizer() + "path: " + pathJoin(parent->myPath, path));
										CAGE_THROW_ERROR(Exception, "file cannot be manipulated, it is opened as archive");
									}
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
	}

	FileAbstract::FileAbstract(const String &path, const FileMode &mode) : myPath(path), myMode(mode)
	{
		CAGE_ASSERT(path == pathSimplify(path));
	}

	void FileAbstract::reopenForModification()
	{
		CAGE_THROW_CRITICAL(Exception, "reopening abstract file");
	}

	void FileAbstract::readAt(PointerRange<char> buffer, uint64 at)
	{
		CAGE_THROW_CRITICAL(Exception, "reading with offset from abstract file");
	}

	FileMode FileAbstract::mode() const
	{
		ScopeLock lock(fsMutex());
		return myMode;
	}

	ArchiveAbstract::ArchiveAbstract(const String &path) : myPath(path)
	{
		CAGE_ASSERT(path == pathSimplify(path));
	}

	ArchiveAbstract::~ArchiveAbstract()
	{
		ScopeLock lock(fsMutex());
		archivesCache().erase(this);
	}

	ArchiveWithPath archiveFindTowardsRoot(const String &path_, ArchiveFindModeEnum mode)
	{
		if (!pathIsValid(path_))
		{
			CAGE_LOG_THROW(Stringizer() + "path: " + path_);
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

		ScopeLock lock(fsMutex());
		const auto ctor = Delegate<std::shared_ptr<ArchiveAbstract>(const String &)>().bind<&archiveOpenReal>();
		std::shared_ptr<ArchiveAbstract> root = archivesCache().getOrCreate(rootPath, ctor).archive;
		ArchiveWithPath r = archiveFindIterate(root, inside, mode);
		switch (mode)
		{
			case ArchiveFindModeEnum::FileExclusive:
				CAGE_ASSERT(r.archive);
				CAGE_ASSERT(!r.path.empty());
				break;
			case ArchiveFindModeEnum::ArchiveExclusive:
				CAGE_ASSERT(r.archive);
				break;
			case ArchiveFindModeEnum::ArchiveShared:
				CAGE_ASSERT(r.archive);
				break;
		}
		return r;
	}

	RecursiveMutex *fsMutex()
	{
		static Holder<RecursiveMutex> *mut = new Holder<RecursiveMutex>(newRecursiveMutex()); // this leak is intentional
		return +*mut;
	}
}
