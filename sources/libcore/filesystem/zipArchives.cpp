#include <cage-core/memoryBuffer.h>
#include <cage-core/concurrent.h>
#include <cage-core/string.h> // StringComparatorFast
#include <cage-core/serialization.h> // bufferView
#include <cage-core/math.h> // min

#include "files.h"

#include <vector>
#include <algorithm>

namespace cage
{
	namespace
	{
		// tables taken from https://en.wikipedia.org/wiki/Zip_(file_format)
		// full description at https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
		// example: https://users.cs.jmu.edu/buchhofp/forensics/formats/pkzip.html

#pragma pack(push, 1)
		struct LocalFileHeader
		{
			uint32 signature = 0x04034b50;
			uint16 versionNeededToExtract = 20;
			uint16 generalPurposeBitFlags = 1 << 11;
			uint16 compressionMethod = 0;
			uint16 lastModificationTime = 0;
			uint16 lastModificationDate = 0;
			uint32 crc32ofUncompressedData = 0;
			uint32 compressedSize = 0;
			uint32 uncompressedSize = 0;
			uint16 nameLength = 0; // bytes
			uint16 extraFieldLength = 0; // bytes
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct CDFileHeader
		{
			uint32 signature = 0x02014b50;
			uint16 versionMadeBy = 20;
			uint16 versionNeededToExtract = 20; // 2.0 support for directories
			uint16 generalPurposeBitFlags = 1 << 11; // utf8 encoded names
			uint16 compressionMethod = 0;
			uint16 lastModificationTime = 0;
			uint16 lastModificationDate = 0;
			uint32 crc32ofUncompressedData = 0;
			uint32 compressedSize = 0;
			uint32 uncompressedSize = 0;
			uint16 nameLength = 0; // bytes
			uint16 extraFieldLength = 0; // bytes
			uint16 commentLength = 0; // bytes
			uint16 diskNumberWhereFileStarts = 0;
			uint16 internalFileAttributes =  0;
			uint32 externalFileAttributes = 0; // 0x10 for directories or 0x80 for files
			uint32 relativeOffsetOfLocalFileHeader = 0;
		};
#pragma pack(pop)

		struct CDFileHeaderEx : public CDFileHeader
		{
			string name;
			MemoryBuffer newContent;
			bool locked = false;
			bool modified = false;

			uintPtr getFileStartOffset() const
			{
				CAGE_ASSERT(!modified);
				return (uintPtr)relativeOffsetOfLocalFileHeader + sizeof(LocalFileHeader) + nameLength + extraFieldLength;
			}
		};

		bool operator < (const CDFileHeaderEx &a, const CDFileHeaderEx &b)
		{
			return StringComparatorFast()(a.name, b.name);
		}

		bool operator < (const CDFileHeaderEx &a, const string &b)
		{
			return StringComparatorFast()(a.name, b);
		}

#pragma pack(push, 1)
		struct EndOfCentralDirectoryRecord
		{
			uint32 signature = 0x06054b50;
			uint16 numberOfThisDisk = 0;
			uint16 diskWhereCDStarts = 0;
			uint16 numberOfCDFRecordsOnThisDisk = 0;
			uint16 totalNumberOfCDFRecords = 0;
			uint32 sizeOfCD = 0; // bytes
			uint32 offsetOfCD = 0; // bytes
			uint16 commentLength = 0; // bytes
		};
#pragma pack(pop)

		LocalFileHeader deriveLocal(const CDFileHeaderEx &e)
		{
			LocalFileHeader l;
			l.versionNeededToExtract = e.versionNeededToExtract;
			l.generalPurposeBitFlags = e.generalPurposeBitFlags;
			l.compressionMethod = e.compressionMethod;
			l.lastModificationTime = e.lastModificationTime;
			l.lastModificationDate = e.lastModificationDate;
			l.crc32ofUncompressedData = e.crc32ofUncompressedData;
			l.compressedSize = e.compressedSize;
			l.uncompressedSize = e.uncompressedSize;
			l.nameLength = e.nameLength;
			l.extraFieldLength = e.extraFieldLength;
			return l;
		}

		bool isPathSafe(const string &path)
		{
			if (pathIsAbs(path))
				return false;
			if (!path.empty() && path[0] == '.')
				return false;
			return true;
		}

		bool isPathValid(const string &path)
		{
			if (path != pathSimplify(path))
				return false;
			return isPathSafe(path);
		}

		class ArchiveZip : public ArchiveAbstract
		{
		public:
			Holder<File> src;
			Holder<Mutex> mutex = newMutex();
			std::vector<CDFileHeaderEx> files;
			bool modified = false;

			// create a new empty archive
			ArchiveZip(const string &path, const string &options) : ArchiveAbstract(path)
			{
				src = writeFile(path);
				modified = true;
			}

			// open existing archive
			ArchiveZip(Holder<File> &&file) : ArchiveAbstract(((FileAbstract*)+file)->myPath), src(templates::move(file))
			{
				CAGE_ASSERT(src->mode().read && src->mode().write && !src->mode().append && !src->mode().textual);

				uint32 totalFiles = m;
				uint32 cdSize = 0;
				{ // validate that file is a valid zip archive
					const uintPtr totalSize = src->size();
					constexpr uintPtr MaxCommentSize = 0;
					constexpr uintPtr MaxSize = MaxCommentSize + sizeof(EndOfCentralDirectoryRecord);
					const uintPtr off = totalSize > MaxSize ? totalSize - MaxSize : 0; // start of the buffer relative to the entire file
					const uintPtr size = min(totalSize, MaxSize);
					src->seek(off);
					const MemoryBuffer b = src->read(size);
					// search for eocd from the back
					for (uint32 i = sizeof(EndOfCentralDirectoryRecord); i <= size; i++)
					{
						const uintPtr pos = size - i; // position relative to start of the buffer
						const char *p = b.data() + pos;
						const EndOfCentralDirectoryRecord &e = *(const EndOfCentralDirectoryRecord *)p;
						constexpr uint32 Signature = EndOfCentralDirectoryRecord().signature;
						if (e.signature != Signature)
							continue; // incorrect signature
						if ((uintPtr)e.offsetOfCD + e.sizeOfCD != off + pos)
							continue; // incorrect header offset
						if (pos + sizeof(EndOfCentralDirectoryRecord) + e.commentLength != size)
							continue; // incorrect comment size
						// we assume that we found an actual ZIP archive now
						if (e.diskWhereCDStarts != 0 || e.numberOfThisDisk != 0 || e.numberOfCDFRecordsOnThisDisk != e.totalNumberOfCDFRecords)
							CAGE_THROW_ERROR(NotImplemented, "cannot read zip archives that span multiple disks");
						totalFiles = e.totalNumberOfCDFRecords;
						cdSize = e.sizeOfCD;
						src->seek(e.offsetOfCD);
						break;
					}
					if (totalFiles == m)
						CAGE_THROW_ERROR(Exception, "file is not a zip archive");
				}

				{ // read central directory (populate files)
					files.reserve(totalFiles);
					MemoryBuffer b = src->read(cdSize);
					Deserializer d(b);
					for (uint32 i = 0; i < totalFiles; i++)
					{
						CDFileHeaderEx e;
						d >> (CDFileHeader &)e;
						constexpr uint32 Signature = CDFileHeader().signature;
						if (e.signature != Signature)
							CAGE_THROW_ERROR(Exception, "invalid signature of a central directory file record in zip archive");
						if (e.diskNumberWhereFileStarts != 0)
							CAGE_THROW_ERROR(Exception, "central directory file record indicates a different disk in zip archive");
						if (e.nameLength >= string::MaxLength)
							CAGE_THROW_ERROR(Exception, "central directory file record name length is too large in zip archive");
						e.name.rawLength() = e.nameLength;
						d.read({ e.name.rawData(), e.name.rawData() + e.nameLength });
						d.advance(e.extraFieldLength);
						d.advance(e.commentLength);

						if (e.externalFileAttributes != 0x80 && e.externalFileAttributes != 0x10)
						{
							CAGE_LOG(SeverityEnum::Warning, "zip", "skipping central directory file record with unknown external file attributes");
							continue;
						}

						e.name = pathSimplify(e.name);
						e.nameLength = e.name.length();

						if (!isPathSafe(e.name))
							CAGE_THROW_ERROR(Exception, "central directory file record name is dangerous in zip archive");

						files.push_back(templates::move(e));
					}
				}

				{ // add records for missing directories
					// todo
					modified = false; // all modifications so far can be safely discarded
				}
			}

			~ArchiveZip()
			{
				CAGE_ASSERT(src);
				if (!modified)
					return;

				// write modified files and update offsets
				src->seek(src->size());
				for (CDFileHeaderEx &f : files)
				{
					CAGE_ASSERT(f.name.length() == f.nameLength);
					CAGE_ASSERT(f.diskNumberWhereFileStarts == 0);
					CAGE_ASSERT(f.commentLength == 0);
					CAGE_ASSERT(f.extraFieldLength == 0);

					if (!f.modified)
						continue;

					CAGE_ASSERT(f.compressionMethod == 0);
					CAGE_ASSERT(f.uncompressedSize == f.newContent.size());
					CAGE_ASSERT(f.uncompressedSize == f.compressedSize || f.compressedSize == 0);
					CAGE_ASSERT(f.externalFileAttributes == 0x10 || f.externalFileAttributes == 0x80);
					CAGE_ASSERT(f.externalFileAttributes == 0x80 || f.uncompressedSize == 0);

					f.relativeOffsetOfLocalFileHeader = numeric_cast<uint32>(src->tell());
					{ // write local file header
						const LocalFileHeader lfh = deriveLocal(f);
						src->write(bufferView(lfh));
						src->write(f.name);
					}
					src->write(f.newContent);
				}

				const uint32 startOfCentralDirectory = numeric_cast<uint32>(src->tell());
				{ // write central directory
					for (const CDFileHeaderEx &f : files)
					{
						src->write(bufferView((CDFileHeader &)f));
						src->write(f.name);
					}
				}

				{ // write end of central directory record
					EndOfCentralDirectoryRecord e;
					e.numberOfCDFRecordsOnThisDisk = e.totalNumberOfCDFRecords = numeric_cast<uint16>(files.size());
					e.offsetOfCD = startOfCentralDirectory;
					e.sizeOfCD = numeric_cast<uint32>(src->tell() - startOfCentralDirectory);
					src->write(bufferView(e));
				}
			}

			uint32 createRecord(const string &path)
			{
				CAGE_ASSERT(isPathValid(path));
				auto it = std::lower_bound(files.begin(), files.end(), path);
				it = files.insert(it, CDFileHeaderEx());
				it->nameLength = path.length();
				it->name = path;
				it->modified = true;
				modified = true;
				return numeric_cast<uint32>(it - files.begin());
			}

			uint32 findRecordIndex(const string &path) const
			{
				CAGE_ASSERT(isPathValid(path));
				const auto it = std::lower_bound(files.begin(), files.end(), path);
				if (it == files.end() || it->name != path)
					return m;
				return numeric_cast<uint32>(it - files.begin());
			}

			void testFileNotLocked(uint32 index) const
			{
				if (index != m && files[index].locked)
					CAGE_THROW_ERROR(Exception, "file is in use");
			}

			void testFileNotLocked(const string &path) const
			{
				const uint32 index = findRecordIndex(path);
				testFileNotLocked(index);
			}

			PathTypeFlags typeNoLock(uint32 index) const
			{
				if (index == m)
					return PathTypeFlags::NotFound;
				const auto &r = files[index];
				if (r.externalFileAttributes & 0x10)
					return PathTypeFlags::Directory;
				return PathTypeFlags::File;
			}

			PathTypeFlags typeNoLock(const string &path) const
			{
				const uint32 index = findRecordIndex(path);
				return typeNoLock(index);
			}

			PathTypeFlags type(const string &path) const override
			{
				ScopeLock l(mutex);
				return typeNoLock(path);
			}

			void createDirectoriesNoLock(const string &path)
			{
				if (path.empty())
					return;
				{ // check existing records
					const uint32 index = findRecordIndex(path);
					switch (typeNoLock(index))
					{
					case PathTypeFlags::Directory:
						return; // directory already exists
					case PathTypeFlags::NotFound:
						break; // ok
					default:
						CAGE_THROW_ERROR(Exception, "cannot create directory inside zip, file already exists");
					}
				}
				{ // create all parents
					const string p = pathJoin(path, "..");
					createDirectoriesNoLock(p);
				}
				{ // create this directory
					const uint32 index = createRecord(path);
					auto &r = files[index];
					r.externalFileAttributes = 0x10;
				}
			}

			void createDirectories(const string &path) override
			{
				ScopeLock l(mutex);
				createDirectoriesNoLock(path);
			}

			void move(const string &from, const string &to) override
			{
				CAGE_ASSERT(isPathValid(from));
				CAGE_ASSERT(isPathValid(to));

				ScopeLock l(mutex);
				uint32 index = findRecordIndex(from);
				if (any(typeNoLock(index) & PathTypeFlags::NotFound))
					CAGE_THROW_ERROR(Exception, "source file or directory does not exist");
				testFileNotLocked(index);
				if (none(typeNoLock(to) & PathTypeFlags::NotFound))
				{
					removeNoLock(to);
					index = findRecordIndex(from);
				}

				CAGE_ASSERT(index != m);
				files[index].name = to;
				files[index].nameLength = to.length();
				modified = true;
			}

			void removeNoLock(const string &path)
			{
				const uint32 index = findRecordIndex(path);
				if (index == m)
					return;
				testFileNotLocked(index);
				files.erase(files.begin() + index);
				modified = true;
			}

			void remove(const string &path) override
			{
				ScopeLock l(mutex);
				removeNoLock(path);
			}

			uint64 lastChange(const string &path) const override
			{
				CAGE_ASSERT(isPathValid(path));
				CAGE_THROW_CRITICAL(NotImplemented, "reading last modification time of a file inside zip archive is not yet supported");
			}

			Holder<File> openFile(const string &path, const FileMode &mode) override;
			Holder<DirectoryList> listDirectory(const string &path) const override;
		};

		Holder<File> newProxyFile(File *f, uintPtr start, uintPtr size)
		{
			// temporarily copy the source into buffer
			// todo actual proxy file
			f->seek(start);
			MemoryBuffer b = f->read(size);
			return newFileBuffer(std::move(b), FileMode(true, false));
		}

		struct FileZip : public FileAbstract
		{
			const std::shared_ptr<ArchiveZip> a;
			const string myName;
			MemoryBuffer buff;
			Holder<File> src;
			bool modified = false;

			FileZip(const std::shared_ptr<ArchiveZip> &archive, const string &name, FileMode mode) : FileAbstract(pathJoin(archive->myPath, name), mode), a(archive), myName(name)
			{
				CAGE_ASSERT(isPathValid(name));
				CAGE_ASSERT(mode.valid());
				CAGE_ASSERT(!mode.append);
				CAGE_ASSERT(!mode.textual);

				ScopeLock l(a->mutex);
				uint32 index = a->findRecordIndex(name);
				if (a->typeNoLock(index) == PathTypeFlags::Directory)
					CAGE_THROW_ERROR(Exception, "cannot open file in zip archive, the path is a directory");
				if (index == m)
				{
					if (mode.read)
						CAGE_THROW_ERROR(Exception, "cannot open file in zip archive, it does not exist");
					a->createDirectoriesNoLock(pathJoin(name, ".."));
					if (index != m)
						index = a->findRecordIndex(name);
					index = a->createRecord(name);
					auto &r = a->files[index];
					r.externalFileAttributes = 0x80;
				}
				else
					a->testFileNotLocked(index);

				CAGE_ASSERT(index != m);
				auto &r = a->files[index];
				CAGE_ASSERT(!r.locked);
				if (r.compressionMethod != 0 && mode.read)
					CAGE_THROW_ERROR(NotImplemented, "reading compressed files from zip archives is not yet supported");
				r.locked = true;

				if (mode.write && !mode.read)
				{ // write only
					// truncate file
					r.newContent.clear();
					r.uncompressedSize = 0;
					r.compressedSize = 0;
					r.modified = true;
					modified = true;
					src = newFileBuffer(&buff);
				}
				else
				{ // read or update
					CAGE_ASSERT(!modified);
					if (r.modified)
						src = newFileBuffer(PointerRange<char>(r.newContent), FileMode(true, false));
					else
						src = newProxyFile(+a->src, r.getFileStartOffset(), r.uncompressedSize);
				}

				CAGE_ASSERT(src);
			}

			~FileZip()
			{
				ScopeLock l(a->mutex);
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
				const uint32 index = a->findRecordIndex(myName);
				CAGE_ASSERT(index != m);
				CDFileHeaderEx &h = a->files[index];
				CAGE_ASSERT(h.locked);
				h.locked = false;
			}

			void read(PointerRange<char> buffer) override
			{
				CAGE_ASSERT(mode.read);
				CAGE_ASSERT(src);
				src->read(buffer);
			}

			void write(PointerRange<const char> buffer) override
			{
				CAGE_ASSERT(mode.write);
				CAGE_ASSERT(src);
				if (!modified)
				{
					const uintPtr pos = src->tell();
					src->seek(0);
					buff = src->readAll();
					src = newFileBuffer(&buff);
					src->seek(pos);
					modified = true;
				}
				src->write(buffer);
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(src);
				src->seek(position);
			}

			void closeNoLock()
			{
				CAGE_ASSERT(modified);
				modified = false; // avoid repeated write from destructor after user's close

				CDFileHeaderEx h;
				h.uncompressedSize = h.compressedSize = numeric_cast<uint32>(buff.size());
				h.externalFileAttributes = 0x80;
				h.name = myName;
				h.nameLength = myName.length();
				h.newContent = templates::move(buff);
				h.locked = true; // unlocked from the destructor only
				h.modified = true;

				const uint32 index = a->findRecordIndex(myName);
				CAGE_ASSERT(index != m);
				CDFileHeaderEx &e = a->files[index];
				std::swap(e, h);

				a->modified = true;
			}

			void close() override
			{
				CAGE_ASSERT(src);
				if (modified)
				{
					ScopeLock l(a->mutex);
					closeNoLock();
				}
			}

			uintPtr tell() const override
			{
				CAGE_ASSERT(src);
				return src->tell();
			}

			uintPtr size() const override
			{
				CAGE_ASSERT(src);
				return src->size();
			}
		};

		struct DirectoryListZip : public DirectoryListAbstract
		{
			const std::shared_ptr<const ArchiveZip> a;
			std::vector<string> names;
			uint32 index = 0;

			DirectoryListZip(const std::shared_ptr<const ArchiveZip> &archive, const string &path) : DirectoryListAbstract(pathJoin(archive->myPath, path)), a(archive)
			{
				CAGE_ASSERT(isPathValid(path));
				ScopeLock l(a->mutex);
				names.reserve(a->files.size());
				for (const auto &it : a->files)
				{
					const string par = pathJoin(it.name, "..");
					if (par == path)
					{
						const string name = pathExtractFilename(it.name);
						names.push_back(name);
					}
				}
			}

			bool valid() const override
			{
				return index < names.size();
			}

			string name() const override
			{
				CAGE_ASSERT(valid());
				return names[index];
			}

			void next() override
			{
				CAGE_ASSERT(valid());
				index++;
			}
		};

		Holder<File> ArchiveZip::openFile(const string &path, const FileMode &mode)
		{
			return detail::systemArena().createImpl<File, FileZip>(std::static_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
		}

		Holder<DirectoryList> ArchiveZip::listDirectory(const string &path) const
		{
			return detail::systemArena().createImpl<DirectoryList, DirectoryListZip>(std::static_pointer_cast<const ArchiveZip>(shared_from_this()), path);
		}
	}

	void archiveCreateZip(const string &path, const string &options)
	{
		ArchiveZip z(path, options);
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenZip(Holder<File> &&f)
	{
		return std::make_shared<ArchiveZip>(templates::move(f));
	}
}

