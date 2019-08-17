#include <vector>
#include <set>
#include <map>

#include "filesystem.h"
#include <cage-core/concurrent.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

#include <zip.h>

namespace cage
{
	archiveVirtual::archiveVirtual(const string &path) : myPath(path)
	{}

	void pathCreateArchive(const string &path, const string &options)
	{
		// someday, switch based on the options may be implemented here to create different types of archives
		// the options are passed on to allow for other options (compression, encryption, ...)
		archiveCreateZip(path, options);
	}

	void mixedMove(std::shared_ptr<archiveVirtual> &af, const string &pf, std::shared_ptr<archiveVirtual> &at, const string &pt)
	{
		{
			holder<fileHandle> f = af ? af->openFile(pf, fileMode(true, false)) : realNewFile(pf, fileMode(true, false));
			holder<fileHandle> t = at ? at->openFile(pt, fileMode(false, true)) : realNewFile(pt, fileMode(false, true));
			// todo split big files into multiple smaller steps
			memoryBuffer b = f->readBuffer(f->size());
			f->close();
			t->writeBuffer(b);
			t->close();
		}
		if (af)
			af->remove(pf);
		else
			realRemove(pf);
	}

	namespace
	{
		std::shared_ptr<archiveVirtual> archiveTryGet(const string &path)
		{
			static holder<syncMutex> *mutex = new holder<syncMutex>(newSyncMutex()); // this leak is intentional
			static std::map<string, std::weak_ptr<archiveVirtual>> *map = new std::map<string, std::weak_ptr<archiveVirtual>>(); // this leak is intentional
			scopeLock<syncMutex> l(*mutex);
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
				detail::overrideException oe;
				auto a = archiveOpenZip(path);
				CAGE_ASSERT(a);
				(*map)[path] = a;
				return a;
			}
			catch (const exception &)
			{
				return {};
			}
		}

		std::shared_ptr<archiveVirtual> recursiveFind(const string &path, bool matchExact, string &insidePath)
		{
			if (matchExact)
			{
				pathTypeFlags t = realType(path);
				if (any(t & pathTypeFlags::File))
					return archiveTryGet(path);
				if (none(t & pathTypeFlags::NotFound))
					return {}; // the path exists (most likely a directory) and is not an archive
			}
			if (pathExtractPathNoDrive(path) == "/")
				return {}; // do not throw an exception by going beyond root
			string a = pathJoin(path, "..");
			string b = path.subString(a.length() + 1, m);
			CAGE_ASSERT(pathJoin(a, b) == path, a, b, path);
			insidePath = pathJoin(b, insidePath);
			return recursiveFind(a, true, insidePath);
		}
	}

	std::shared_ptr<archiveVirtual> archiveFindTowardsRoot(const string &path, bool matchExact, string &insidePath)
	{
		CAGE_ASSERT(insidePath.empty());
		return recursiveFind(pathToAbs(path), matchExact, insidePath);
	}

	namespace
	{
#define LOCK scopeLock<syncMutex> lock(mutex)

		class archiveZip : public archiveVirtual
		{
		public:
			holder<syncMutex> mutex;
			std::vector<memoryBuffer> writtenBuffers;
			zip_t *zip;

			// create archive
			archiveZip(const string &path, const string &options) : archiveVirtual(path), zip(nullptr)
			{
				mutex = newSyncMutex();
				realCreateDirectories(pathJoin(path, ".."));
				zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_EXCL, nullptr);
				if (!zip)
					CAGE_THROW_ERROR(exception, "zip_open");
				static const string comment = "Archive created by Cage";
				zip_set_archive_comment(zip, comment.c_str(), comment.length());
				zip_dir_add(zip, "", 0);
			}

			// open archive
			archiveZip(const string &path) : archiveVirtual(path), zip(nullptr)
			{
				mutex = newSyncMutex();
				if (newFile(path, fileMode(true, false))->size() == 0)
					CAGE_THROW_ERROR(exception, "empty file"); // this is a temporary workaround until it is improved in the libzip
				zip = zip_open(path.c_str(), ZIP_CHECKCONS, nullptr);
				if (!zip)
					CAGE_THROW_ERROR(exception, "zip_open");
			}

			~archiveZip()
			{
				LOCK;
				if (zip_close(zip) != 0)
					CAGE_LOG(severityEnum::Error, "exception", "failed to close zip archive. all changes may have been lost");
				zip = nullptr;
			}

			pathTypeFlags typeNoLock(const string &path)
			{
				// first, try to detect folder
				if (!path.empty() && path[path.length() - 1] != '/')
				{
					pathTypeFlags f = typeNoLock(path + "/");
					if ((f & pathTypeFlags::Directory) == pathTypeFlags::Directory)
						return f;
				}
				zip_stat_t st;
				if (zip_stat(zip, path.c_str(), 0, &st) != 0)
					return pathTypeFlags::NotFound | pathTypeFlags::InsideArchive;
				if ((st.valid & ZIP_STAT_NAME) == 0)
					return pathTypeFlags::NotFound | pathTypeFlags::InsideArchive;
				string n = st.name;
				if (n.empty())
					return pathTypeFlags::NotFound | pathTypeFlags::InsideArchive;
				if (n[n.length() - 1] == '/')
					return pathTypeFlags::Directory | pathTypeFlags::InsideArchive;
				return pathTypeFlags::File | pathTypeFlags::InsideArchive;
			}

			pathTypeFlags type(const string &path) override
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
						if ((typeNoLock(p) & pathTypeFlags::Directory) == pathTypeFlags::Directory)
							continue;
						if (zip_dir_add(zip, p.c_str(), ZIP_FL_ENC_UTF_8) < 0)
							CAGE_THROW_ERROR(exception, "zip_dir_add");
					}
				}
			}

			void move(const string &from, const string &to) override
			{
				LOCK;
				auto i = zip_name_locate(zip, from.c_str(), 0);
				if (i < 0)
					CAGE_THROW_ERROR(exception, "zip_name_locate");
				if (zip_file_rename(zip, i, to.c_str(), ZIP_FL_ENC_UTF_8) != 0)
					CAGE_THROW_ERROR(exception, "zip_file_rename");
			}

			void remove(const string &path) override
			{
				LOCK;
				auto i = zip_name_locate(zip, path.c_str(), 0);
				if (i < 0)
					CAGE_THROW_ERROR(exception, "zip_name_locate");
				if (zip_delete(zip, i) != 0)
					CAGE_THROW_ERROR(exception, "zip_delete");
			}

			uint64 lastChange(const string &path) override
			{
				LOCK;
				CAGE_THROW_CRITICAL(notImplemented, "lastChange is not yet implemented for zip archives");
			}

			holder<fileHandle> openFile(const string &path, const fileMode &mode) override;
			holder<directoryList> listDirectory(const string &path) override;
		};

#undef LOCK
#define LOCK scopeLock<syncMutex> lock(a->mutex)

		struct fileZipRead : public fileVirtual
		{
			std::shared_ptr<archiveZip> a;
			zip_file_t *f;

			fileZipRead(const std::shared_ptr<archiveZip> &archive, const string &name, fileMode mode) : fileVirtual(name, mode), a(archive), f(nullptr)
			{
				LOCK;
				f = zip_fopen(a->zip, name.c_str(), 0);
				if (!f)
					CAGE_THROW_ERROR(exception, "zip_fopen");
			}

			~fileZipRead()
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

			void read(void *data, uint64 size) override
			{
				LOCK;
				CAGE_ASSERT(f);
				if (zip_fread(f, data, size) != size)
					CAGE_THROW_ERROR(exception, "zip_fread");
			}

			void write(const void *data, uint64 size) override
			{
				CAGE_THROW_CRITICAL(notImplemented, "calling write on read-only zip file");
			}

			void seek(uint64 position) override
			{
				LOCK;
				CAGE_ASSERT(f);
				if (zip_fseek(f, position, 0) != 0)
					CAGE_THROW_ERROR(exception, "zip_fseek");
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
				if (zip_fclose(ff) != 0)
					CAGE_THROW_ERROR(exception, "zip_fclose");
			}

			uint64 tell() const override
			{
				LOCK;
				CAGE_ASSERT(f);
				auto r = zip_ftell(f);
				if (r < 0)
					CAGE_THROW_ERROR(exception, "zip_ftell");
				return r;
			}

			uint64 size() const override
			{
				LOCK;
				CAGE_ASSERT(f);
				zip_stat_t st;
				if (zip_stat(a->zip, myPath.c_str(), 0, &st) != 0)
					CAGE_THROW_ERROR(exception, "zip_stat");
				if ((st.valid & ZIP_STAT_SIZE) == 0)
					CAGE_THROW_ERROR(exception, "zip_stat");
				return st.size;
			}
		};

		struct fileZipWrite : public fileVirtual
		{
			std::shared_ptr<archiveZip> a;
			memoryBuffer m;
			serializer s;
			bool closed;

			fileZipWrite(const std::shared_ptr<archiveZip> &archive, const string &name, fileMode mode) : fileVirtual(name, mode), a(archive), s(m), closed(false)
			{}

			~fileZipWrite()
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

			void read(void *data, uint64 size) override
			{
				CAGE_THROW_CRITICAL(notImplemented, "calling read on write-only zip file");
			}

			void write(const void *data, uint64 size) override
			{
				CAGE_ASSERT(!closed);
				s.write(data, size);
			}

			void seek(uint64 position) override
			{
				CAGE_THROW_CRITICAL(notImplemented, "calling seek on write-only zip file");
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
					CAGE_THROW_ERROR(exception, "zip_source_buffer");
				a->writtenBuffers.push_back(templates::move(m)); // the buffer itself must survive until the entire archive is closed
				auto i = zip_file_add(a->zip, myPath.c_str(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
				if (i < 0)
				{
					zip_source_free(src);
					CAGE_THROW_ERROR(exception, "zip_file_add");
				}
				if (zip_set_file_compression(a->zip, i, ZIP_CM_STORE, 0) != 0)
					CAGE_THROW_ERROR(exception, "zip_set_file_compression");
			}

			uint64 tell() const override
			{
				CAGE_ASSERT(!closed);
				return m.size();
			}

			uint64 size() const override
			{
				CAGE_ASSERT(!closed);
				return m.size();
			}
		};

		struct directoryListZip : public directoryListVirtual
		{
			std::shared_ptr<archiveZip> a;
			std::vector<string> names;
			uint32 index;

			directoryListZip(const std::shared_ptr<archiveZip> &archive, const string &path) : directoryListVirtual(pathJoin(archive->myPath, path)), a(archive), index(0)
			{
				LOCK;
				auto cnt = zip_get_num_entries(a->zip, 0);
				if (cnt < 0)
					CAGE_THROW_ERROR(exception, "zip_get_num_entries");
				std::set<string> nn;
				for (uint32 i = 0; i < cnt; i++)
				{
					auto n = zip_get_name(a->zip, i, 0);
					if (!n)
					{
						if (zip_get_error(a->zip)->zip_err == ZIP_ER_DELETED)
							continue;
						CAGE_THROW_ERROR(exception, "zip_get_name");
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

		holder<fileHandle> archiveZip::openFile(const string &path, const fileMode &mode)
		{
			CAGE_ASSERT(!path.empty());
			CAGE_ASSERT(mode.valid());
			CAGE_ASSERT(mode.read != mode.write, "zip archive cannot open file for both reading and writing simultaneously");
			CAGE_ASSERT(!mode.append, "zip archive cannot open file for append");
			createDirectories(pathJoin(path, ".."));
			if (mode.read)
				return detail::systemArena().createImpl<fileHandle, fileZipRead>(std::dynamic_pointer_cast<archiveZip>(shared_from_this()), path, mode);
			else
				return detail::systemArena().createImpl<fileHandle, fileZipWrite>(std::dynamic_pointer_cast<archiveZip>(shared_from_this()), path, mode);
		}

		holder<directoryList> archiveZip::listDirectory(const string &path)
		{
			createDirectories(path);
			return detail::systemArena().createImpl<directoryList, directoryListZip>(std::dynamic_pointer_cast<archiveZip>(shared_from_this()), path);
		}
	}

	void archiveCreateZip(const string &path, const string &options)
	{
		archiveZip a(path, options);
	}

	std::shared_ptr<archiveVirtual> archiveOpenZip(const string &path)
	{
		return std::make_shared<archiveZip>(path);
	}
}

