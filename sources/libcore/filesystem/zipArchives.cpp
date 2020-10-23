#include <cage-core/memoryBuffer.h>
#include <cage-core/concurrent.h>
#include <cage-core/string.h>
#include <cage-core/serialization.h>

#include "files.h"

#include <cstring>
#include <vector>
#include <set>

#include <zip.h>

namespace cage
{
	namespace
	{
#define LOCK ScopeLock<Mutex> lock(mutex)

		string zipErr(int code)
		{
			zip_error_t t;
			zip_error_init_with_code(&t, code);
			string s = zip_error_strerror(&t);
			zip_error_fini(&t);
			return s;
		}

		class ArchiveZip : public ArchiveAbstract
		{
		public:
			Holder<Mutex> mutex = newMutex();
			std::vector<MemoryBuffer> writtenBuffers;
			zip_t *zip = nullptr;

			ArchiveZip(const string &path) : ArchiveAbstract(path)
			{}

			void finalize()
			{
				LOCK;
				if (zip_close(zip) != 0)
				{
					CAGE_LOG(SeverityEnum::Error, "exception", "failed to close zip archive. all changes may have been lost");
					zip_discard(zip); // free the memory
				}
				zip = nullptr;
			}

			void initCreated()
			{
				CAGE_ASSERT(zip);
				constexpr const char *comment = "Archive created by Cage";
				zip_set_archive_comment(zip, comment, numeric_cast<uint16>(std::strlen(comment)));
				zip_dir_add(zip, "", 0);
			}

			PathTypeFlags typeNoLock(const string &path)
			{
				// first, try to detect folder
				if (!path.empty() && path[path.length() - 1] != '/')
				{
					PathTypeFlags f = typeNoLock(path + "/");
					if (any(f & PathTypeFlags::Directory))
						return f;
				}
				zip_stat_t st;
				if (zip_stat(zip, path.c_str(), 0, &st) != 0)
					return PathTypeFlags::NotFound | PathTypeFlags::InsideArchive;
				if ((st.valid & ZIP_STAT_NAME) == 0)
					return PathTypeFlags::NotFound | PathTypeFlags::InsideArchive;
				string n = st.name;
				if (n.empty())
					return PathTypeFlags::NotFound | PathTypeFlags::InsideArchive;
				if (n[n.length() - 1] == '/')
					return PathTypeFlags::Directory | PathTypeFlags::InsideArchive;
				return PathTypeFlags::File | PathTypeFlags::InsideArchive;
			}

			PathTypeFlags type(const string &path) override
			{
				LOCK;
				return typeNoLock(path);
			}

			void createDirectories(const string &path) override
			{
				if (path.empty())
					return;
				LOCK;
				const string pth = path + "/";
				uint32 off = 0;
				while (true)
				{
					uint32 pos = find(subString(pth, off, m), '/');
					if (pos == m)
						return; // done
					pos += off;
					off = pos + 1;
					if (pos)
					{
						const string p = subString(pth, 0, pos);
						if (any(typeNoLock(p) & PathTypeFlags::Directory))
							continue;
						if (zip_dir_add(zip, p.c_str(), ZIP_FL_ENC_UTF_8) < 0)
							CAGE_THROW_ERROR(Exception, "zip_dir_add");
					}
				}
			}

			void move(const string &from, const string &to) override
			{
				LOCK;
				auto i = zip_name_locate(zip, from.c_str(), 0);
				if (i < 0)
					CAGE_THROW_ERROR(Exception, "zip_name_locate");
				if (zip_file_rename(zip, i, to.c_str(), ZIP_FL_ENC_UTF_8) != 0)
					CAGE_THROW_ERROR(Exception, "zip_file_rename");
			}

			void remove(const string &path) override
			{
				LOCK;
				auto i = zip_name_locate(zip, path.c_str(), 0);
				if (i < 0)
					CAGE_THROW_ERROR(Exception, "zip_name_locate");
				if (zip_delete(zip, i) != 0)
					CAGE_THROW_ERROR(Exception, "zip_delete");
			}

			uint64 lastChange(const string &path) override
			{
				LOCK;
				CAGE_THROW_CRITICAL(NotImplemented, "lastChange is not yet implemented for zip archives");
			}

			Holder<File> openFile(const string &path, const FileMode &mode) override;
			Holder<DirectoryList> listDirectory(const string &path) override;
		};

		class ArchiveZipHandle : public ArchiveZip
		{
		public:
			Holder<File> under;
			zip_source_t *src = nullptr;

			zip_int64_t callback(char *data, zip_uint64_t len, zip_source_cmd_t cmd)
			{
				//CAGE_LOG(SeverityEnum::Info, "zip-callback", stringizer() + "cmd: " + cmd + ", data: " + (uintPtr)data + ", len: " + len);
				switch (cmd)
				{
				case ZIP_SOURCE_SUPPORTS:
					return zip_source_make_command_bitmap(
						ZIP_SOURCE_SUPPORTS,
						ZIP_SOURCE_STAT,
						ZIP_SOURCE_ERROR,
						//ZIP_SOURCE_ACCEPT_EMPTY,
						ZIP_SOURCE_OPEN,
						ZIP_SOURCE_BEGIN_WRITE,
						ZIP_SOURCE_READ,
						ZIP_SOURCE_WRITE,
						ZIP_SOURCE_SEEK,
						ZIP_SOURCE_SEEK_WRITE,
						ZIP_SOURCE_TELL,
						ZIP_SOURCE_TELL_WRITE,
						ZIP_SOURCE_CLOSE,
						ZIP_SOURCE_COMMIT_WRITE,
						ZIP_SOURCE_ROLLBACK_WRITE,
						ZIP_SOURCE_REMOVE,
						-1
					);

				case ZIP_SOURCE_STAT:
				{
					zip_stat_t &s = *(zip_stat_t *)data;
					zip_stat_init(&s);
					s.size = under->size();
					s.valid |= ZIP_STAT_SIZE;
					return 0;
				}

				case ZIP_SOURCE_ERROR:
					return 0; // 2 * sizeof(int);

				case ZIP_SOURCE_ACCEPT_EMPTY:
					return 0; // we do not accept empty file as valid zip archive

					// io

				case ZIP_SOURCE_OPEN:
					return 0;

				case ZIP_SOURCE_BEGIN_WRITE:
					return 0;

				case ZIP_SOURCE_READ:
					under->read({ data, data + len });
					return len;

				case ZIP_SOURCE_WRITE:
					under->write({ data, data + len });
					return len;

				case ZIP_SOURCE_SEEK:
				case ZIP_SOURCE_SEEK_WRITE:
					under->seek(zip_source_seek_compute_offset(under->tell(), under->size(), data, len, nullptr));
					return 0;

				case ZIP_SOURCE_TELL:
				case ZIP_SOURCE_TELL_WRITE:
					return under->tell();

				case ZIP_SOURCE_CLOSE:
					return 0;

				case ZIP_SOURCE_COMMIT_WRITE:
					return 0;

					// required but unsupported

				case ZIP_SOURCE_ROLLBACK_WRITE:
				case ZIP_SOURCE_REMOVE:
					break;
				}
				return -1; // error
			}

			static zip_int64_t globalCallback(void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd)
			{
				try
				{
					return ((ArchiveZipHandle *)userdata)->callback((char*)data, len, cmd);
				}
				catch (const Exception &)
				{
					return -1; // error
				}
				catch (...)
				{
					detail::logCurrentCaughtException();
					CAGE_THROW_CRITICAL(Exception, "unknown exception in zip source callback");
				}
			}

			// create archive
			explicit ArchiveZipHandle(Holder<File> &&f, const string &path, const string &options) : ArchiveZip(path)
			{
				pathCreateDirectories(pathJoin(path, ".."));
				under = templates::move(f);
				src = zip_source_function_create(&globalCallback, this, nullptr);
				if (!src)
					CAGE_THROW_ERROR(Exception, "zip_source_function_create");
				zip = zip_open_from_source(src, ZIP_CREATE, nullptr);
				if (!zip)
				{
					zip_source_free(src);
					src = nullptr;
					CAGE_THROW_ERROR(Exception, "zip_open_from_source");
				}
				initCreated();
			}

			// open archive
			explicit ArchiveZipHandle(Holder<File> &&f, const string &path) : ArchiveZip(path)
			{
				under = templates::move(f);
				src = zip_source_function_create(&globalCallback, this, nullptr);
				if (!src)
					CAGE_THROW_ERROR(Exception, "zip_source_function_create");
				zip = zip_open_from_source(src, ZIP_CHECKCONS, nullptr);
				if (!zip)
				{
					zip_source_free(src);
					src = nullptr;
					CAGE_THROW_ERROR(Exception, "zip_open_from_source");
				}
			}

			~ArchiveZipHandle()
			{
				// ensure that all changes are applied before any of the local variables goes out of scope
				finalize();
			}
		};

		class ArchiveZipReal : public ArchiveZip
		{
		public:
			// create archive
			explicit ArchiveZipReal(const string &path, const string &options) : ArchiveZip(path)
			{
				realCreateDirectories(pathJoin(path, ".."));
				int err = 0;
				zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_EXCL, &err);
				if (!zip)
					CAGE_THROW_ERROR(Exception, "failed to create a zip archive");
				initCreated();
			}

			// open archive
			explicit ArchiveZipReal(const string &path) : ArchiveZip(path)
			{
				int err = 0;
				zip = zip_open(path.c_str(), ZIP_CHECKCONS, &err);
				if (!zip)
					CAGE_THROW_ERROR(Exception, "failed to open a zip archive");
			}

			~ArchiveZipReal()
			{
				// ensure that all changes are applied before any of the local variables goes out of scope
				finalize();
			}
		};

#undef LOCK
#define LOCK ScopeLock<Mutex> lock(a->mutex)

		struct FileZipRead : public FileAbstract
		{
			const std::shared_ptr<ArchiveZip> a;
			zip_file_t *f = nullptr;

			FileZipRead(const std::shared_ptr<ArchiveZip> &archive, const string &name, FileMode mode) : FileAbstract(name, mode), a(archive)
			{
				CAGE_ASSERT(mode.read && !mode.write);
				LOCK;
				f = zip_fopen(a->zip, name.c_str(), 0);
				if (!f)
					CAGE_THROW_ERROR(Exception, "failed to open a file inside zip archive");
			}

			~FileZipRead()
			{
				if (f)
				{
					try
					{
						close();
					}
					catch (...)
					{
						// do nothing
					}
				}
			}

			void read(PointerRange<char> buffer) override
			{
				LOCK;
				CAGE_ASSERT(f);
				if (zip_fread(f, buffer.data(), buffer.size()) != buffer.size())
					CAGE_THROW_ERROR(Exception, "zip_fread");
			}

			void seek(uintPtr position) override
			{
				LOCK;
				CAGE_ASSERT(f);
				if (zip_fseek(f, position, 0) != 0)
					CAGE_THROW_ERROR(Exception, "zip_fseek");
			}

			void close() override
			{
				LOCK;
				CAGE_ASSERT(f);
				auto ff = f;
				f = nullptr; // avoid double free
				int err = zip_fclose(ff);
				if (err != 0)
				{
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "error: " + zipErr(err));
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "file path: '" + myPath + "'");
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "archive path: '" + a->myPath + "'");
					CAGE_THROW_ERROR(Exception, "failed to close a file inside zip archive");
				}
			}

			uintPtr tell() const override
			{
				LOCK;
				CAGE_ASSERT(f);
				auto r = zip_ftell(f);
				if (r < 0)
					CAGE_THROW_ERROR(Exception, "zip_ftell");
				return numeric_cast<uintPtr>(r);
			}

			uintPtr size() const override
			{
				LOCK;
				CAGE_ASSERT(f);
				zip_stat_t st;
				if (zip_stat(a->zip, myPath.c_str(), 0, &st) != 0)
					CAGE_THROW_ERROR(Exception, "zip_stat");
				if ((st.valid & ZIP_STAT_SIZE) == 0)
					CAGE_THROW_ERROR(Exception, "zip_stat");
				return numeric_cast<uintPtr>(st.size);
			}
		};

		struct FileZipWrite : public FileAbstract
		{
			const std::shared_ptr<ArchiveZip> a;
			MemoryBuffer mb;
			Holder<File> f;

			FileZipWrite(const std::shared_ptr<ArchiveZip> &archive, const string &name, FileMode mode) : FileAbstract(name, mode), a(archive)
			{
				CAGE_ASSERT(!mode.read && mode.write);
				f = newFileBuffer(&mb);
			}

			~FileZipWrite()
			{
				if (f)
				{
					try
					{
						close();
					}
					catch (...)
					{
						// do nothing
					}
				}
			}

			void write(PointerRange<const char> buffer) override
			{
				CAGE_ASSERT(f);
				f->write(buffer);
			}

			void seek(uintPtr position) override
			{
				f->seek(position);
			}

			void close() override
			{
				LOCK;
				CAGE_ASSERT(f);
				Holder<File> ff;
				std::swap(f, ff); // ensure close
				zip_source_t *src = zip_source_buffer(a->zip, mb.data(), mb.size(), 0);
				if (!src)
					CAGE_THROW_ERROR(Exception, "zip_source_buffer");
				a->writtenBuffers.push_back(templates::move(mb)); // the buffer itself must survive until the entire archive is closed
				auto i = zip_file_add(a->zip, myPath.c_str(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
				if (i < 0)
				{
					zip_source_free(src);
					CAGE_THROW_ERROR(Exception, "zip_file_add");
				}
				if (zip_set_file_compression(a->zip, i, ZIP_CM_STORE, 0) != 0)
					CAGE_THROW_ERROR(Exception, "zip_set_file_compression");
			}

			uintPtr tell() const override
			{
				CAGE_ASSERT(f);
				return f->tell();
			}

			uintPtr size() const override
			{
				CAGE_ASSERT(f);
				return f->size();
			}
		};

		struct FileZipRW : public FileAbstract
		{
			const string name;
			const std::shared_ptr<ArchiveZip> a;
			Holder<File> r;
			Holder<File> w;
			uintPtr originalSize = 0;
			uintPtr position = 0;

			FileZipRW(const std::shared_ptr<ArchiveZip> &archive, const string &name, FileMode mode) : FileAbstract(name, mode), name(name), a(archive)
			{
				CAGE_ASSERT(mode.read && mode.write);
				zip_stat_t st;
				if (zip_stat(a->zip, name.c_str(), 0, &st) != 0)
					CAGE_THROW_ERROR(Exception, "zip_stat");
				CAGE_ASSERT(st.flags & ZIP_STAT_SIZE);
				originalSize = st.size;
			}

			void read(PointerRange<char> buffer) override
			{
				CAGE_ASSERT(!r || !w);
				if (w)
					CAGE_THROW_CRITICAL(Exception, "cannot read from zip archive file after writing to it");
				if (!r)
				{
					r = a->openFile(name, FileMode(true, false));
					if (position)
						r->seek(position);
				}
				r->read(buffer);
			}

			void write(PointerRange<const char> buffer) override
			{
				CAGE_ASSERT(!r || !w);
				if (!w)
				{
					if (r)
					{
						position = r->tell();
						r->seek(0);
					}
					else
						r = a->openFile(name, FileMode(true, false));
					w = a->openFile(name, FileMode(false, true));
					w->write(r->readAll());
					w->seek(position);
					r->close();
					r.clear();
				}
				w->write(buffer);
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(!r || !w);
				if (w)
					w->seek(position);
				else if (r)
					r->seek(position);
				else
					this->position = position;
			}

			void close() override
			{
				CAGE_ASSERT(!r || !w);
				if (r)
				{
					r->close();
					r.clear();
				}
				if (w)
				{
					originalSize = w->size();
					w->close();
					w.clear();
				}
				position = 0;
			}

			uintPtr tell() const override
			{
				CAGE_ASSERT(!r || !w);
				if (w)
					return w->tell();
				else if (r)
					return r->tell();
				else
					return position;
			}

			uintPtr size() const override
			{
				CAGE_ASSERT(!r || !w);
				if (w)
					return w->size();
				else if (r)
					return r->size();
				else
					return originalSize;
			}
		};

		struct DirectoryListZip : public DirectoryListAbstract
		{
			const std::shared_ptr<ArchiveZip> a;
			std::vector<string> names;
			uint32 index = 0;

			DirectoryListZip(const std::shared_ptr<ArchiveZip> &archive, const string &path) : DirectoryListAbstract(pathJoin(archive->myPath, path)), a(archive)
			{
				LOCK;
				auto cnt = zip_get_num_entries(a->zip, 0);
				if (cnt < 0)
					CAGE_THROW_ERROR(Exception, "zip_get_num_entries");
				std::set<string> nn;
				for (uint32 i = 0; i < cnt; i++)
				{
					auto n = zip_get_name(a->zip, i, 0);
					if (!n)
					{
						if (zip_get_error(a->zip)->zip_err == ZIP_ER_DELETED)
							continue;
						CAGE_THROW_ERROR(Exception, "zip_get_name");
					}
					if (n[0] == 0)
						continue;
					string s = n;
					if (s[s.length() - 1] == '/')
						s = subString(s, 0, s.length() - 1);
					if (pathExtractPath(s) == path)
					{
						s = pathExtractFilename(s);
						if (!s.empty())
							nn.insert(s);
					}
				}
				names.reserve(nn.size());
				names.insert(names.end(), nn.begin(), nn.end());
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
				index++;
			}
		};

		Holder<File> ArchiveZip::openFile(const string &path, const FileMode &mode)
		{
			CAGE_ASSERT(!path.empty());
			CAGE_ASSERT(mode.valid());
			CAGE_ASSERT(mode.read || mode.write);
			CAGE_ASSERT(!mode.append);
			CAGE_ASSERT(!mode.textual);
			createDirectories(pathJoin(path, ".."));
			if (mode.read && mode.write)
				return detail::systemArena().createImpl<File, FileZipRW>(std::static_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
			else if (mode.read)
				return detail::systemArena().createImpl<File, FileZipRead>(std::static_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
			else
				return detail::systemArena().createImpl<File, FileZipWrite>(std::static_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
		}

		Holder<DirectoryList> ArchiveZip::listDirectory(const string &path)
		{
			createDirectories(path);
			return detail::systemArena().createImpl<DirectoryList, DirectoryListZip>(std::static_pointer_cast<ArchiveZip>(shared_from_this()), path);
		}
	}

	void archiveCreateZip(const string &path, const string &options)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, false, p);
		if (a)
		{
			ArchiveZipHandle z(a->openFile(p, FileMode(false, true)), path, options);
		}
		else
		{
			ArchiveZipReal z(path, options);
		}
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenZip(const string &path)
	{
		return std::make_shared<ArchiveZipReal>(path);
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenZip(Holder<File> &&f, const string &path)
	{
		return std::make_shared<ArchiveZipHandle>(templates::move(f), path);
	}
}

