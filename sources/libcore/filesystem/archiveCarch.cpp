#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "files.h"

#include <cage-core/flatSet.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h> // bufferView
#include <cage-core/stdHash.h>
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
		struct ArchiveHeader
		{
			char magic[12] = "cageArchive";
			uint32 version = 0;
			uint32 filesCount = 0;
			uint32 dirsCount = 0;
			uint64 listStart = 0;
			uint64 listSize = 0;
			uint64 contentStart = 0;
			uint64 contentSize = 0;
		};

		// preceded by name
		struct FileHeader
		{
			uint64 offset = 0; // from the contentStart position
			uint64 size = 0;
		};

		struct FileHeaderEx : public FileHeader
		{
			MemoryBuffer newContent;
			bool locked = false;
			bool modified = false;
		};

		bool isPathValid(const String &path)
		{
			if (path != pathSimplify(path))
				return false;
			if (pathIsAbs(path))
				return false;
			if (!path.empty() && path[0] == '.')
				return false;
			return true;
		}

		class ArchiveCarch final : public ArchiveAbstract
		{
		public:
			Holder<File> src;
			std::unordered_map<String, FileHeaderEx> files;
			std::unordered_set<String> dirs;
			ArchiveHeader arch;
			bool modified = false;
			bool isCarch = false;

			// create a new empty archive
			ArchiveCarch(const String &path) : ArchiveAbstract(path)
			{
				src = writeFile(path);
				modified = true;
				isCarch = true;
			}

			// open existing archive
			ArchiveCarch(Holder<File> &&file) : ArchiveAbstract(class_cast<FileAbstract *>(+file)->myPath), src(std::move(file))
			{
				CAGE_ASSERT(src->mode().read && !src->mode().write && !src->mode().append && !src->mode().textual);

				{ // read header
					if (src->size() < sizeof(ArchiveHeader))
						return; // isCarch = false
					src->seek(0);
					src->read(bufferView<char>(arch));
					if (String(ArchiveHeader().magic) != String(arch.magic))
						return; // isCarch = false
					if (ArchiveHeader().version != arch.version)
						return; // isCarch = false
					isCarch = true;
				}

				{ // validate content size
					if (src->size() < arch.contentStart + arch.contentSize)
					{
						CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
						CAGE_THROW_ERROR(Exception, "truncated cage archive (content)");
					}
				}

				{ // read files list
					if (src->size() < arch.listStart + arch.listSize)
					{
						CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
						CAGE_THROW_ERROR(Exception, "truncated cage archive (list)");
					}
					src->seek(arch.listStart);
					const auto buff = src->read(arch.listSize);
					Deserializer des(buff);
					files.reserve(arch.filesCount);
					for (uint32 i = 0; i < arch.filesCount; i++)
					{
						String n;
						FileHeader h;
						des >> n >> h;
						if (arch.contentSize < h.offset + h.size)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
							CAGE_LOG_THROW(Stringizer() + "name: " + n);
							CAGE_THROW_ERROR(Exception, "truncated cage archive (file)");
						}
						(FileHeader &)files[n] = h;
					}
					dirs.reserve(arch.dirsCount);
					for (uint32 i = 0; i < arch.dirsCount; i++)
					{
						String n;
						des >> n;
						dirs.insert(n);
					}
				}
			}

			ArchiveCarch(ArchiveCarch &&z) = default;

			~ArchiveCarch()
			{
				if (!src)
					return; // already moved-from

				ScopeLock lock(fsMutex());
				if (!modified)
					return;
				CAGE_ASSERT(src->mode().write);
				CAGE_ASSERT(isCarch);

				// prepare new list
				std::vector<std::pair<String, FileHeaderEx>> fs;
				fs.reserve(files.size());
				for (auto &it : files)
					fs.emplace_back(it.first, std::move(it.second));
				std::sort(fs.begin(), fs.end(), [](const auto &a, const auto &b) -> bool { return StringComparatorFast()(a.first, b.first); });

				// read previous content
				// this is inefficient and temporary solution
				// todo
				for (auto &it : fs)
				{
					if (!it.second.modified)
					{
						it.second.newContent.resize(it.second.size);
						src->seek(arch.contentStart + it.second.offset);
						src->read(it.second.newContent);
					}
				}

				// update offsets
				{
					uint64 off = 0;
					for (auto &it : fs)
					{
						it.second.offset = off;
						off += it.second.size;
					}
					arch.contentSize = off;
				}

				// prepare list buffer
				MemoryBuffer buff;
				buff.reserve(fs.size() * (sizeof(FileHeader) + 100));
				Serializer ser(buff);
				{
					for (const auto &it : fs)
						ser << it.first << (FileHeader)it.second;
				}
				{
					std::vector<String> ds;
					ds.reserve(dirs.size());
					for (const auto &it : dirs)
						ds.push_back(it);
					std::sort(ds.begin(), ds.end(), StringComparatorFast());
					for (const auto &it : ds)
						ser << it;
				}

				// write all headers
				arch.listStart = sizeof(ArchiveHeader);
				arch.listSize = buff.size();
				arch.contentStart = arch.listStart + arch.listSize;
				arch.filesCount = fs.size();
				arch.dirsCount = dirs.size();
				src->seek(0);
				src->write(bufferView(arch));
				src->write(buff);

				// write all content
				for (const auto &it : fs)
				{
					CAGE_ASSERT(it.second.size == it.second.newContent.size());
					src->write(it.second.newContent);
				}

				{ // fill the rest of the file with zeros
					static constexpr uint64 sz = 16 * 1024;
					static constexpr char zeros[sz] = {};
					uint64 r = src->size() - src->tell();
					while (r > sz)
					{
						src->write(zeros);
						r -= sz;
					}
					if (r > 0)
						src->write({ zeros, zeros + sz });
				}

				// ensure we wrote to the very end of the file
				CAGE_ASSERT(src->tell() == src->size());

				// make sure the archive is released while the lock is still held
				src->close();
				src.clear();
			}

			void reopenForModification()
			{
				ScopeLock lock(fsMutex());
				if (src->mode().write)
					return; // already modifiable
				class_cast<FileAbstract *>(+src)->reopenForModification();
			}

			void throwIfFileLocked(const String &path) const
			{
				if (path.empty())
					return;
				auto it = files.find(path);
				if (it == files.end())
					return;
				if (it->second.locked)
				{
					CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
					CAGE_LOG_THROW(Stringizer() + "name: " + it->first);
					CAGE_THROW_ERROR(Exception, "file is in use");
				}
			}

			PathTypeFlags type(const String &path) const override
			{
				CAGE_ASSERT(isPathValid(path));
				ScopeLock lock(fsMutex());
				if (files.count(path))
					return PathTypeFlags::File;
				if (dirs.count(path))
					return PathTypeFlags::Directory;
				return PathTypeFlags::NotFound;
			}

			void createDirectoriesNoLock(const String &path)
			{
				if (path.empty())
					return;
				// check existing records
				if (files.count(path))
				{
					CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
					CAGE_LOG_THROW(Stringizer() + "name: " + path);
					CAGE_THROW_ERROR(Exception, "cannot create directory inside carch, file already exists");
				}
				if (dirs.count(path))
					return; // directory already exists
				reopenForModification();
				modified = true;
				{ // create all parents
					const String p = pathJoin(path, "..");
					createDirectoriesNoLock(p);
				}
				dirs.insert(path);
			}

			void createDirectories(const String &path) override
			{
				CAGE_ASSERT(isPathValid(path));
				ScopeLock lock(fsMutex());
				createDirectoriesNoLock(path);
			}

			void move(const String &from, const String &to, bool copying) override
			{
				CAGE_ASSERT(!from.empty());
				CAGE_ASSERT(!to.empty());
				CAGE_ASSERT(isPathValid(from));
				CAGE_ASSERT(isPathValid(to));
				if (from == to)
					return;
				ScopeLock lock(fsMutex());
				if (files.count(from))
				{
					Holder<PointerRange<char>> buff = openFile(from, FileMode(true, false))->readAll();
					openFile(to, FileMode(false, true))->write(buff);
				}
				else if (dirs.count(from))
				{
					const auto list = listDirectory(from);
					for (const auto &it : list)
					{
						const String name = pathExtractFilename(it);
						move(pathJoin(from, name), pathJoin(to, name), copying);
					}
				}
				else
				{
					CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
					CAGE_LOG_THROW(Stringizer() + "name: " + from);
					CAGE_THROW_ERROR(Exception, "source does not exist");
				}
				if (!copying)
					remove(from);
			}

			void remove(const String &path) override
			{
				CAGE_ASSERT(!path.empty());
				CAGE_ASSERT(isPathValid(path));
				ScopeLock lock(fsMutex());
				if (files.count(path))
				{
					reopenForModification();
					throwIfFileLocked(path);
					files.erase(path);
					modified = true;
					return;
				}
				if (dirs.count(path))
				{
					reopenForModification();
					// todo locked folders?
					dirs.erase(path);
					modified = true;
					// todo remove recursively?
				}
			}

			PathLastChange lastChange(const String &path) const override
			{
				CAGE_ASSERT(!path.empty());
				CAGE_ASSERT(isPathValid(path));
				CAGE_THROW_CRITICAL(Exception, "reading last modification time of a file inside carch archive is not supported");
			}

			Holder<File> openFile(const String &path, const FileMode &mode) override;
			Holder<PointerRange<String>> listDirectory(const String &path) const override;
		};

		// file contained within the zip archive
		struct FileCarch final : public FileAbstract
		{
			std::shared_ptr<ArchiveCarch> a;
			const String myName; // name inside the archive
			MemoryBuffer buff;
			Holder<File> src;
			bool modified = false;

			FileCarch(const std::shared_ptr<ArchiveCarch> &archive, const String &name, FileMode mode) : FileAbstract(pathJoin(archive->myPath, name), mode), a(archive), myName(name)
			{
				CAGE_ASSERT(!name.empty());
				CAGE_ASSERT(isPathValid(name));
				CAGE_ASSERT(mode.valid());
				CAGE_ASSERT(!mode.append);

				ScopeLock lock(fsMutex());
				if (mode.write)
					a->reopenForModification();
				if (a->dirs.count(name))
					CAGE_THROW_ERROR(Exception, "cannot open file in carch archive, the path is a directory");
				if (a->files.count(name))
					a->throwIfFileLocked(name);
				else
				{
					if (mode.read)
						CAGE_THROW_ERROR(Exception, "cannot open file in carch archive, it does not exist");
					a->createDirectoriesNoLock(pathJoin(name, ".."));
				}

				auto &r = a->files[name];
				CAGE_ASSERT(!r.locked);
				r.locked = true;

				if (mode.write && !mode.read)
				{ // write only
					r.newContent.clear(); // truncate file
					r.modified = true;
					modified = true;
					src = newFileBuffer(Holder<MemoryBuffer>(&buff, nullptr));
				}
				else
				{ // read or update
					CAGE_ASSERT(!modified);
					if (r.modified)
						src = newFileBuffer(systemMemory().createHolder<PointerRange<char>>(PointerRange<char>(r.newContent)), FileMode(true, false));
					else
						src = newProxyFile(+a->src, a->arch.contentStart + r.offset, r.size);
				}

				CAGE_ASSERT(src);
			}

			~FileCarch()
			{
				ScopeLock lock(fsMutex());
				if (src && modified)
				{
					try
					{
						closeNoLock();
					}
					catch (const cage::Exception &)
					{
						// do nothing
					}
				}
				CAGE_ASSERT(a->files.count(myName));
				auto &r = a->files[myName];
				CAGE_ASSERT(r.locked);
				r.locked = false;
				a.reset(); // make sure the archive itself is released while the lock is still held
			}

			void reopenForModificationInternal()
			{
				CAGE_ASSERT(!modified);
				a->reopenForModification();
				const uintPtr pos = src->tell();
				src->seek(0);
				buff.resize(src->size());
				src->read(buff);
				src = newFileBuffer(Holder<MemoryBuffer>(&buff, nullptr));
				src->seek(pos);
				modified = true;
			}

			void reopenForModification() override
			{
				CAGE_ASSERT(!modified);
				CAGE_ASSERT(!myMode.write);
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(src);
				reopenForModificationInternal();
				myMode.write = true;
			}

			void readAt(PointerRange<char> buffer, uintPtr at) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(myMode.read);
				CAGE_ASSERT(src);
				class_cast<FileAbstract *>(+src)->readAt(buffer, at);
			}

			void read(PointerRange<char> buffer) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(myMode.read);
				CAGE_ASSERT(src);
				src->read(buffer);
			}

			void write(PointerRange<const char> buffer) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(myMode.write);
				CAGE_ASSERT(src);
				if (!modified)
					reopenForModificationInternal();
				src->write(buffer);
			}

			void seek(uintPtr position) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(src);
				src->seek(position);
			}

			void closeNoLock()
			{
				CAGE_ASSERT(modified);
				modified = false; // avoid repeated write from destructor after user's close

				CAGE_ASSERT(a->files.count(myName));
				auto &r = a->files[myName];
				CAGE_ASSERT(r.locked);
				r.modified = true;
				r.size = buff.size();
				r.newContent = std::move(buff);
				a->modified = true;
			}

			void close() override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(src);
				src.clear();
				if (modified)
					closeNoLock();
			}

			uintPtr tell() override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(src);
				return src->tell();
			}

			uintPtr size() override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(src);
				return src->size();
			}
		};

		Holder<File> ArchiveCarch::openFile(const String &path, const FileMode &mode)
		{
			ScopeLock lock(fsMutex());
			try
			{
				return systemMemory().createImpl<File, FileCarch>(std::static_pointer_cast<ArchiveCarch>(shared_from_this()), path, mode);
			}
			catch (...)
			{
				CAGE_LOG_THROW(Stringizer() + "archive path: " + myPath);
				CAGE_LOG_THROW(Stringizer() + "name: " + path);
				throw;
			}
		}

		Holder<PointerRange<String>> ArchiveCarch::listDirectory(const String &path) const
		{
			CAGE_ASSERT(isPathValid(path));
			ScopeLock lock(fsMutex());
			FlatSet<String> ns;
			for (const auto &it : files)
			{
				const String par = pathJoin(it.first, "..");
				if (par == path)
					ns.insert(pathJoin(myPath, it.first));
			}
			for (const auto &it : dirs)
			{
				const String par = pathJoin(it, "..");
				if (par == path)
					ns.insert(pathJoin(myPath, it));
			}
			PointerRangeHolder<String> names(ns.begin(), ns.end());
			return names;
		}
	}

	void archiveCreateCarch(const String &path)
	{
		ScopeLock lock(fsMutex());
		ArchiveCarch z(path);
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenCarchTry(Holder<File> f)
	{
		ScopeLock lock(fsMutex());
		ArchiveCarch z(std::move(f));
		return z.isCarch ? std::make_shared<ArchiveCarch>(std::move(z)) : std::shared_ptr<ArchiveAbstract>();
	}
}
