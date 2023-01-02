#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/concurrent.h>
#include <cage-core/string.h> // StringComparatorFast
#include <cage-core/serialization.h> // bufferView
#include <cage-core/math.h> // min

#include "files.h"

#include <algorithm>
#include <zlib.h> // crc32_z

namespace cage
{
	namespace
	{
		// provides a limited section of a file as another file
		struct ProxyFile : public FileAbstract
		{
			FileAbstract *f = nullptr;
			const uintPtr start;
			const uintPtr capacity;
			uintPtr off = 0;

			ProxyFile(FileAbstract *f, uintPtr start, uintPtr capacity) : FileAbstract(f->myPath, FileMode(true, false)), f(f), start(start), capacity(capacity)
			{}

			~ProxyFile()
			{}

			void readAt(PointerRange<char> buffer, uintPtr at) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(f);
				CAGE_ASSERT(buffer.size() <= capacity - at);
				f->readAt(buffer, start + at);
			}

			void read(PointerRange<char> buffer) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(f);
				CAGE_ASSERT(buffer.size() <= capacity - off);
				f->seek(start + off);
				f->read(buffer);
				off += buffer.size();
			}

			void seek(uintPtr position) override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(f);
				CAGE_ASSERT(position <= capacity);
				off = position;
			}

			void close() override
			{
				ScopeLock lock(fsMutex());
				f = nullptr;
			}

			uintPtr tell() override
			{
				ScopeLock lock(fsMutex());
				return off;
			}

			uintPtr size() override
			{
				ScopeLock lock(fsMutex());
				return capacity;
			}
		};

		Holder<File> newProxyFile(File *f, uintPtr start, uintPtr size)
		{
			return systemMemory().createImpl<File, ProxyFile>(class_cast<FileAbstract *>(f), start, size);
		}
	}

	namespace
	{
		// structures: https://en.wikipedia.org/wiki/Zip_(file_format)
		// full description: https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
		// external file attributes: https://docs.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants
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

		struct CDFileHeaderEx : public CDFileHeader
		{
			String name;
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

		bool operator < (const CDFileHeaderEx &a, const String &b)
		{
			return StringComparatorFast()(a.name, b);
		}

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

		bool isPathSafe(const String &path)
		{
			if (pathIsAbs(path))
				return false;
			if (!path.empty() && path[0] == '.')
				return false;
			return true;
		}

		bool isPathValid(const String &path)
		{
			if (path != pathSimplify(path))
				return false;
			return isPathSafe(path);
		}

		class ArchiveZip : public ArchiveAbstract
		{
		public:
			Holder<File> src;
			std::vector<CDFileHeaderEx> files;
			uint32 originalCDFilesPosition = 0;
			uint32 originalEOCDPosition = 0;
			bool modified = false;
			bool isZip = false;

			// create a new empty archive
			ArchiveZip(const String &path, const String &options) : ArchiveAbstract(path)
			{
				src = writeFile(path);
				modified = true;
			}

			// open existing archive
			ArchiveZip(Holder<File> &&file, bool throwing = true) : ArchiveAbstract(class_cast<FileAbstract *>(+file)->myPath), src(std::move(file))
			{
				CAGE_ASSERT(src->mode().read && !src->mode().write && !src->mode().append && !src->mode().textual);

				uint32 totalFiles = m;
				uint32 cdSize = 0;
				{ // validate that file is a valid zip archive
					const uintPtr totalSize = src->size();
					static constexpr uintPtr MaxCommentSize = 0;
					static constexpr uintPtr MaxSize = MaxCommentSize + sizeof(EndOfCentralDirectoryRecord);
					const uintPtr off = totalSize > MaxSize ? totalSize - MaxSize : 0; // start of the buffer relative to the entire file
					const uintPtr size = min(totalSize, MaxSize);
					src->seek(off);
					Holder<PointerRange<char>> b = src->read(size);
					// search for eocd from the back
					for (uint32 i = sizeof(EndOfCentralDirectoryRecord); i <= size; i++)
					{
						const uintPtr pos = size - i; // position relative to start of the buffer
						const char *p = b.data() + pos;
						const EndOfCentralDirectoryRecord &e = *(const EndOfCentralDirectoryRecord *)p;
						static constexpr uint32 Signature = EndOfCentralDirectoryRecord().signature;
						if (e.signature != Signature)
							continue; // incorrect signature
						if ((uintPtr)e.offsetOfCD + e.sizeOfCD != off + pos)
							continue; // incorrect header offset
						if (pos + sizeof(EndOfCentralDirectoryRecord) + e.commentLength != size)
							continue; // incorrect comment size
						// we assume that we found an actual ZIP archive now
						isZip = true;
						if (e.diskWhereCDStarts != 0 || e.numberOfThisDisk != 0 || e.numberOfCDFRecordsOnThisDisk != e.totalNumberOfCDFRecords)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_THROW_ERROR(NotImplemented, "cannot read zip archives that span multiple disks");
						}
						totalFiles = e.totalNumberOfCDFRecords;
						cdSize = e.sizeOfCD;
						originalCDFilesPosition = e.offsetOfCD;
						originalEOCDPosition = e.offsetOfCD + e.sizeOfCD;
						break;
					}
					if (totalFiles == m)
					{
						if (throwing)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_THROW_ERROR(Exception, "file is not a zip archive");
						}
						else
							return;
					}
				}

				{ // read central directory (populate files)
					src->seek(originalCDFilesPosition); // seek to where the list of files is
					files.reserve(totalFiles);
					Holder<PointerRange<char>> b = src->read(cdSize);
					Deserializer d(b);
					for (uint32 i = 0; i < totalFiles; i++)
					{
						CDFileHeaderEx e;
						d >> (CDFileHeader &)e;
						static constexpr uint32 Signature = CDFileHeader().signature;
						if (e.signature != Signature)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG_THROW(Stringizer() + "position in archive: " + src->tell());
							CAGE_THROW_ERROR(Exception, "zip file record has invalid signature");
						}
						if (e.nameLength >= String::MaxLength)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG_THROW(Stringizer() + "position in archive: " + src->tell());
							CAGE_THROW_ERROR(Exception, "zip file record name length is too large");
						}
						e.name.rawLength() = e.nameLength;
						d.read({ e.name.rawData(), e.name.rawData() + e.nameLength });
						if (e.diskNumberWhereFileStarts != 0)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG_THROW(Stringizer() + "name: '" + e.name + "'");
							CAGE_THROW_ERROR(Exception, "zip file record uses a different disk");
						}
						if (e.uncompressedSize == m || e.compressedSize == m)
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG_THROW(Stringizer() + "name: '" + e.name + "'");
							CAGE_THROW_ERROR(NotImplemented, "cannot read file, zip64 not yet supported");
						}
						e.name = pathSimplify(e.name);
						e.nameLength = e.name.length();
						if (!isPathSafe(e.name))
						{
							CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG_THROW(Stringizer() + "name: '" + e.name + "'");
							CAGE_THROW_ERROR(Exception, "zip file record name is dangerous");
						}
						if (e.extraFieldLength)
						{
							CAGE_LOG(SeverityEnum::Note, "zip", Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG(SeverityEnum::Note, "zip", Stringizer() + "name: " + e.name);
							CAGE_LOG(SeverityEnum::Warning, "zip", "skipping zip file extra fields");
							d.read(e.extraFieldLength);
							e.extraFieldLength = 0;
						}
						if (e.commentLength)
						{
							CAGE_LOG(SeverityEnum::Note, "zip", Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG(SeverityEnum::Note, "zip", Stringizer() + "name: " + e.name);
							CAGE_LOG(SeverityEnum::Warning, "zip", "skipping zip file comment field");
							d.read(e.commentLength);
							e.commentLength = 0;
						}

						e.externalFileAttributes &= ~0x20; // suppress FILE_ATTRIBUTE_ARCHIVE
						e.externalFileAttributes &= ~0x800; // suppress FILE_ATTRIBUTE_COMPRESSED
						if (e.externalFileAttributes == 0)
							e.externalFileAttributes = 0x80; // make it FILE_ATTRIBUTE_NORMAL
						if (e.externalFileAttributes != 0x80 && e.externalFileAttributes != 0x10)
						{
							CAGE_LOG(SeverityEnum::Note, "zip", Stringizer() + "archive path: '" + myPath + "'");
							CAGE_LOG(SeverityEnum::Note, "zip", Stringizer() + "name: '" + e.name + "'");
							CAGE_LOG(SeverityEnum::Warning, "zip", Stringizer() + "skipping zip file record with unknown external file attributes: " + e.externalFileAttributes);
							continue;
						}

						files.push_back(std::move(e));
					}
					std::sort(files.begin(), files.end());
				}

				{ // add records for missing directories
					// todo
				}

				modified = false; // all modifications so far can be safely discarded
			}

			~ArchiveZip()
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(src);
				CAGE_ASSERT(src->mode().write == modified);
				if (!modified)
					return;

				{ // overwrite previous CD files list and EOCD record to reduce confusion of some unzip programs
					src->seek(originalCDFilesPosition);

					// in case some files were removed, we may not have enough data to fill the file to the end (and no truncate), therefore, we fill some of the space with zeros
					uint32 expectedToWrite = 0;
					for (const CDFileHeaderEx &f : files)
						expectedToWrite += numeric_cast<uint32>(sizeof(CDFileHeader) + f.nameLength + f.newContent.size());
					const uint32 needToWrite = originalEOCDPosition - originalCDFilesPosition;
					if (needToWrite > expectedToWrite)
					{
						MemoryBuffer buff;
						buff.resize(needToWrite - expectedToWrite);
						detail::memset(buff.data(), 0, buff.size());
						src->write(buff);
					}
				}

				// write modified files and update offsets
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
					CAGE_ASSERT(f.uncompressedSize == f.compressedSize);
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

				// ensure we wrote to the very end of the file
				CAGE_ASSERT(src->tell() == src->size());
			}

			void reopenForModification()
			{
				ScopeLock lock(fsMutex());
				if (src->mode().write)
					return; // already modifiable
				class_cast<FileAbstract *>(+src)->reopenForModification();
			}

			uint32 createRecord(const String &path)
			{
				CAGE_ASSERT(isPathValid(path));
				CAGE_ASSERT(findRecordIndex(path) == m); // make sure the path does not exist yet
				auto it = std::lower_bound(files.begin(), files.end(), path);
				it = files.insert(it, CDFileHeaderEx());
				it->nameLength = path.length();
				it->name = path;
				it->modified = true;
				modified = true;
				return numeric_cast<uint32>(it - files.begin());
			}

			uint32 findRecordIndex(const String &path) const
			{
				CAGE_ASSERT(isPathValid(path));
				const auto it = std::lower_bound(files.begin(), files.end(), path);
				if (it == files.end() || it->name != path)
					return m;
				return numeric_cast<uint32>(it - files.begin());
			}

			void throwIfFileLocked(uint32 index) const
			{
				if (index != m && files[index].locked)
					CAGE_THROW_ERROR(Exception, "file is in use");
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

			PathTypeFlags type(const String &path) const override
			{
				CAGE_ASSERT(isPathValid(path));
				ScopeLock lock(fsMutex());
				const uint32 index = findRecordIndex(path);
				return typeNoLock(index);
			}

			void createDirectoriesNoLock(const String &path)
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
					{
						CAGE_LOG_THROW(Stringizer() + "archive path: '" + myPath + "'");
						CAGE_LOG_THROW(Stringizer() + "name: '" + path + "'");
						CAGE_THROW_ERROR(Exception, "cannot create directory inside zip, file already exists");
					}
					}
				}
				{ // create all parents
					const String p = pathJoin(path, "..");
					createDirectoriesNoLock(p);
				}
				{ // create this directory
					const uint32 index = createRecord(path);
					auto &r = files[index];
					r.externalFileAttributes = 0x10;
				}
			}

			void createDirectories(const String &path) override
			{
				CAGE_ASSERT(isPathValid(path));
				ScopeLock lock(fsMutex());
				reopenForModification();
				createDirectoriesNoLock(path);
			}

			void move(const String &from, const String &to) override
			{
				CAGE_ASSERT(!from.empty());
				CAGE_ASSERT(!to.empty());
				CAGE_ASSERT(isPathValid(from));
				CAGE_ASSERT(isPathValid(to));
				if (from == to)
					return;
				ScopeLock lock(fsMutex());
				// todo optimize this
				Holder<PointerRange<char>> buff = openFile(from, FileMode(true, false))->readAll();
				openFile(to, FileMode(false, true))->write(buff);
				remove(from);
			}

			void remove(const String &path) override
			{
				CAGE_ASSERT(!path.empty());
				CAGE_ASSERT(isPathValid(path));
				ScopeLock lock(fsMutex());
				reopenForModification();
				const uint32 index = findRecordIndex(path);
				if (index == m)
					return;
				throwIfFileLocked(index);
				files.erase(files.begin() + index);
				modified = true;
			}

			PathLastChange lastChange(const String &path) const override
			{
				CAGE_ASSERT(!path.empty());
				CAGE_ASSERT(isPathValid(path));
				CAGE_THROW_CRITICAL(NotImplemented, "reading last modification time of a file inside zip archive is not yet supported");
			}

			Holder<File> openFile(const String &path, const FileMode &mode) override;
			Holder<PointerRange<String>> listDirectory(const String &path) const override;
		};

		// file contained within the zip archive
		struct FileZip : public FileAbstract
		{
			std::shared_ptr<ArchiveZip> a;
			const String myName; // name inside the archive
			MemoryBuffer buff;
			Holder<File> src;
			bool modified = false;

			FileZip(const std::shared_ptr<ArchiveZip> &archive, const String &name, FileMode mode) : FileAbstract(pathJoin(archive->myPath, name), mode), a(archive), myName(name)
			{
				CAGE_ASSERT(!name.empty());
				CAGE_ASSERT(isPathValid(name));
				CAGE_ASSERT(mode.valid());
				CAGE_ASSERT(!mode.append);

				ScopeLock lock(fsMutex());
				if (mode.write)
					a->reopenForModification();
				uint32 index = a->findRecordIndex(name);
				if (a->typeNoLock(index) == PathTypeFlags::Directory)
					CAGE_THROW_ERROR(Exception, "cannot open file in zip archive, the path is a directory");
				if (index == m)
				{
					if (mode.read)
						CAGE_THROW_ERROR(Exception, "cannot open file in zip archive, it does not exist");
					a->createDirectoriesNoLock(pathJoin(name, ".."));
					index = a->createRecord(name);
					auto &r = a->files[index];
					r.externalFileAttributes = 0x80;
				}
				else
					a->throwIfFileLocked(index);

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
					src = newFileBuffer(Holder<MemoryBuffer>(&buff, nullptr));
				}
				else
				{ // read or update
					CAGE_ASSERT(!modified);
					if (r.modified)
						src = newFileBuffer(systemMemory().createHolder<PointerRange<char>>(PointerRange<char>(r.newContent)), FileMode(true, false));
					else
						src = newProxyFile(+a->src, r.getFileStartOffset(), r.uncompressedSize);
				}

				CAGE_ASSERT(src);
			}

			~FileZip()
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
				const uint32 index = a->findRecordIndex(myName);
				CAGE_ASSERT(index != m);
				CDFileHeaderEx &h = a->files[index];
				CAGE_ASSERT(h.locked);
				h.locked = false;
				a.reset(); // make sure the archive itself is released while the lock is still held
			}

			void reopenForModificationInternal()
			{
				CAGE_ASSERT(!modified);
				a->reopenForModification();
				const uintPtr pos = src->tell();
				src->seek(0);
				{
					buff.resize(src->size());
					Holder<PointerRange<char>> tmp = src->readAll();
					CAGE_ASSERT(tmp.size() == buff.size());
					detail::memcpy(buff.data(), tmp.data(), buff.size());
				}
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

				CDFileHeaderEx h;
				h.uncompressedSize = h.compressedSize = numeric_cast<uint32>(buff.size());
				h.externalFileAttributes = 0x80;
				h.name = myName;
				h.nameLength = myName.length();
				h.newContent = std::move(buff);
				{
					uint32 crc = crc32_z(0, nullptr, 0);
					crc = crc32_z(crc, (unsigned char *)h.newContent.data(), h.newContent.size());
					h.crc32ofUncompressedData = crc;
				}
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

		Holder<File> ArchiveZip::openFile(const String &path, const FileMode &mode)
		{
			ScopeLock lock(fsMutex());
			return systemMemory().createImpl<File, FileZip>(std::static_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
		}

		Holder<PointerRange<String>> ArchiveZip::listDirectory(const String &path) const
		{
			CAGE_ASSERT(isPathValid(path));
			ScopeLock lock(fsMutex());
			PointerRangeHolder<String> names;
			names.reserve(files.size());
			for (const auto &it : files)
			{
				const String par = pathJoin(it.name, "..");
				if (par == path)
					names.push_back(pathJoin(myPath, it.name));
			}
			return names;
		}
	}

	void archiveCreateZip(const String &path, const String &options)
	{
		ScopeLock lock(fsMutex());
		ArchiveZip z(path, options);
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenZipTry(Holder<File> &&f)
	{
		ScopeLock lock(fsMutex());
		auto a = std::make_shared<ArchiveZip>(std::move(f), false);
		return a->isZip ? a : std::shared_ptr<ArchiveAbstract>();
	}
}
