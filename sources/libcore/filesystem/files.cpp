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
		struct ArchivesCache : private Immovable
		{
			Holder<Mutex> mutex = newMutex();

			std::shared_ptr<ArchiveAbstract> tryGet(const string &path) const
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

			std::shared_ptr<ArchiveAbstract> assign(std::shared_ptr<ArchiveAbstract> a)
			{
				CAGE_ASSERT(!tryGet(a->myPath));
				map[a->myPath] = a;
				return a;
			}

		private:
			std::map<string, std::weak_ptr<ArchiveAbstract>, StringComparatorFast> map;
		};

		ArchivesCache &archivesCache()
		{
			static ArchivesCache *cache = new ArchivesCache(); // this leak is intentional
			return *cache;
		}

		struct ArchiveWithExisting
		{
			std::shared_ptr<ArchiveAbstract> archive;
			bool existing = false;
		};

		ArchiveWithExisting archiveTryGet(const std::shared_ptr<ArchiveAbstract> &parent, const string &inPath)
		{
			CAGE_ASSERT(parent);
			const string fullPath = pathJoin(parent->myPath, inPath);
			CAGE_ASSERT(fullPath == pathSimplify(fullPath));
			{
				auto a = archivesCache().tryGet(fullPath);
				if (a)
					return { a, true };
			}
			try
			{
				std::shared_ptr<ArchiveAbstract> a;
				{
					detail::OverrideException oe;
					a = archiveOpenZip(parent->openFile(inPath, FileMode(true, false)));
				}
				CAGE_ASSERT(a);
				a = archivesCache().assign(a);
				return { a, false };
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
					ArchiveWithExisting b = archiveTryGet(parent, p);
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

		ScopeLock lock(archivesCache().mutex);

		std::shared_ptr<ArchiveAbstract> root = [&]() -> std::shared_ptr<ArchiveAbstract>
		{
			auto a = archivesCache().tryGet(rootPath);
			if (a)
				return a;
			return archivesCache().assign(archiveOpenReal(rootPath));
		}();

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
}
