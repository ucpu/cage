#include "filesystem.h"
#include <cage-core/concurrent.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/debug.h>

#include <zip.h>

#include <vector>
#include <set>
#include <map>
#include <cstring>

namespace cage
{
	ArchiveAbstract::ArchiveAbstract(const string &path) : myPath(path)
	{}

	void pathCreateArchive(const string &path, const string &options)
	{
		// someday, switch based on the options may be implemented here to create different types of archives
		// the options are passed on to allow for other options (compression, encryption, ...)
		archiveCreateZip(path, options);
	}

	void mixedMove(std::shared_ptr<ArchiveAbstract> &af, const string &pf, std::shared_ptr<ArchiveAbstract> &at, const string &pt)
	{
		{
			Holder<File> f = af ? af->openFile(pf, FileMode(true, false)) : realNewFile(pf, FileMode(true, false));
			Holder<File> t = at ? at->openFile(pt, FileMode(false, true)) : realNewFile(pt, FileMode(false, true));
			// todo split big files into multiple smaller steps
			MemoryBuffer b = f->readAll();
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
		std::shared_ptr<ArchiveAbstract> archiveTryGet(const string &path)
		{
			static Holder<Mutex> *mutex = new Holder<Mutex>(newMutex()); // this leak is intentional
			static std::map<string, std::weak_ptr<ArchiveAbstract>> *map = new std::map<string, std::weak_ptr<ArchiveAbstract>>(); // this leak is intentional
			ScopeLock<Mutex> l(*mutex);
			auto it = map->find(path);
			if (it != map->end())
			{
				auto a = it->second.lock();
				if (a)
					return a;
			}
			// todo determine archive type of the file
			try
			{
				detail::OverrideException oe;
				auto a = archiveOpenZip(path);
				CAGE_ASSERT(a);
				(*map)[path] = a;
				return a;
			}
			catch (const Exception &)
			{
				return {};
			}
		}

		std::shared_ptr<ArchiveAbstract> recursiveFind(const string &path, bool matchExact, string &insidePath)
		{
			CAGE_ASSERT(path == pathSimplify(path));
			if (matchExact)
			{
				PathTypeFlags t = realType(path);
				if (any(t & PathTypeFlags::File))
					return archiveTryGet(path);
				if (none(t & PathTypeFlags::NotFound))
					return {}; // the path exists (most likely a directory) and is not an archive
			}
			if (pathExtractPathNoDrive(path) == "/")
				return {}; // do not throw an exception by going beyond root
			string a = pathJoin(path, "..");
			string b = path.subString(a.length() + 1, m);
			CAGE_ASSERT(pathJoin(a, b) == path);
			insidePath = pathJoin(b, insidePath);
			return recursiveFind(a, true, insidePath);
		}
	}

	std::shared_ptr<ArchiveAbstract> archiveFindTowardsRoot(const string &path, bool matchExact, string &insidePath)
	{
		CAGE_ASSERT(insidePath.empty());
		return recursiveFind(pathToAbs(path), matchExact, insidePath);
	}

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

			// create archive
			ArchiveZip(const string &path, const string &options) : ArchiveAbstract(path)
			{
				realCreateDirectories(pathJoin(path, ".."));
				int err = 0;
				zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_EXCL, &err);
				if (!zip)
				{
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "error: '" + zipErr(err) + "'");
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "options: '" + options + "'");
					CAGE_THROW_ERROR(Exception, "failed to create a zip archive");
				}
				static constexpr const char *comment = "Archive created by Cage";
				zip_set_archive_comment(zip, comment, numeric_cast<uint16>(std::strlen(comment)));
				zip_dir_add(zip, "", 0);
			}

			// open archive
			ArchiveZip(const string &path) : ArchiveAbstract(path)
			{
				int err = 0;
				zip = zip_open(path.c_str(), ZIP_CHECKCONS, &err);
				if (!zip)
				{
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "error: '" + zipErr(err) + "'");
					CAGE_THROW_ERROR(Exception, "failed to open a zip archive");
				}
			}

			~ArchiveZip()
			{
				LOCK;
				if (zip_close(zip) != 0)
				{
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "error: " + zip_strerror(zip));
					CAGE_LOG(SeverityEnum::Error, "exception", "failed to close zip archive. all changes may have been lost");
					zip_discard(zip); // free the memory
				}
				zip = nullptr;
			}

			PathTypeFlags typeNoLock(const string &path)
			{
				// first, try to detect folder
				if (!path.empty() && path[path.length() - 1] != '/')
				{
					PathTypeFlags f = typeNoLock(path + "/");
					if ((f & PathTypeFlags::Directory) == PathTypeFlags::Directory)
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
				LOCK;
				string pth = path + "/";
				uint32 off = 0;
				while (true)
				{
					uint32 pos = pth.subString(off, m).find('/');
					if (pos == m)
						return; // done
					pos += off;
					off = pos + 1;
					if (pos)
					{
						const string p = pth.subString(0, pos);
						if ((typeNoLock(p) & PathTypeFlags::Directory) == PathTypeFlags::Directory)
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

#undef LOCK
#define LOCK ScopeLock<Mutex> lock(a->mutex)

		struct FileZipRead : public FileAbstract
		{
			std::shared_ptr<ArchiveZip> a;
			zip_file_t *f;

			FileZipRead(const std::shared_ptr<ArchiveZip> &archive, const string &name, FileMode mode) : FileAbstract(name, mode), a(archive), f(nullptr)
			{
				LOCK;
				f = zip_fopen(a->zip, name.c_str(), 0);
				if (!f)
				{
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "error: " + zip_strerror(a->zip));
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "file path: '" + name + "'");
					//CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "archive path: '" + a->myPath + "'");
					CAGE_THROW_ERROR(Exception, "failed to open a file inside zip archive");
				}
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

			void read(void *data, uintPtr size) override
			{
				LOCK;
				CAGE_ASSERT(f);
				if (zip_fread(f, data, size) != size)
					CAGE_THROW_ERROR(Exception, "zip_fread");
			}

			void write(const void *data, uintPtr size) override
			{
				CAGE_THROW_CRITICAL(NotImplemented, "calling write on read-only zip file");
			}

			void seek(uintPtr position) override
			{
				LOCK;
				CAGE_ASSERT(f);
				if (zip_fseek(f, position, 0) != 0)
					CAGE_THROW_ERROR(Exception, "zip_fseek");
			}

			void flush() override
			{
				// ignored
			}

			void close() override
			{
				LOCK;
				CAGE_ASSERT(f);
				auto ff = f;
				f = nullptr;
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
			std::shared_ptr<ArchiveZip> a;
			MemoryBuffer m;
			Serializer s;
			bool closed;

			FileZipWrite(const std::shared_ptr<ArchiveZip> &archive, const string &name, FileMode mode) : FileAbstract(name, mode), a(archive), s(m), closed(false)
			{}

			~FileZipWrite()
			{
				if (!closed)
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

			void read(void *data, uintPtr size) override
			{
				CAGE_THROW_CRITICAL(NotImplemented, "calling read on write-only zip file");
			}

			void write(const void *data, uintPtr size) override
			{
				CAGE_ASSERT(!closed);
				s.write({ (const char *)data, (const char *)data + size });
			}

			void seek(uintPtr position) override
			{
				CAGE_THROW_CRITICAL(NotImplemented, "calling seek on write-only zip file");
			}

			void flush() override
			{
				// ignored
			}

			void close() override
			{
				LOCK;
				CAGE_ASSERT(!closed);
				closed = true;
				zip_source_t *src = zip_source_buffer(a->zip, m.data(), m.size(), 0);
				if (!src)
					CAGE_THROW_ERROR(Exception, "zip_source_buffer");
				a->writtenBuffers.push_back(templates::move(m)); // the buffer itself must survive until the entire archive is closed
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
				CAGE_ASSERT(!closed);
				return m.size();
			}

			uintPtr size() const override
			{
				CAGE_ASSERT(!closed);
				return m.size();
			}
		};

		struct DirectoryListZip : public DirectoryListAbstract
		{
			std::shared_ptr<ArchiveZip> a;
			std::vector<string> names;
			uint32 index;

			DirectoryListZip(const std::shared_ptr<ArchiveZip> &archive, const string &path) : DirectoryListAbstract(pathJoin(archive->myPath, path)), a(archive), index(0)
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
						s = s.subString(0, s.length() - 1);
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
			CAGE_ASSERT(mode.read != mode.write);
			CAGE_ASSERT(!mode.append);
			createDirectories(pathJoin(path, ".."));
			if (mode.read)
				return detail::systemArena().createImpl<File, FileZipRead>(std::dynamic_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
			else
				return detail::systemArena().createImpl<File, FileZipWrite>(std::dynamic_pointer_cast<ArchiveZip>(shared_from_this()), path, mode);
		}

		Holder<DirectoryList> ArchiveZip::listDirectory(const string &path)
		{
			createDirectories(path);
			return detail::systemArena().createImpl<DirectoryList, DirectoryListZip>(std::dynamic_pointer_cast<ArchiveZip>(shared_from_this()), path);
		}
	}

	void archiveCreateZip(const string &path, const string &options)
	{
		ArchiveZip a(path, options);
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenZip(const string &path)
	{
		return std::make_shared<ArchiveZip>(path);
	}
}

