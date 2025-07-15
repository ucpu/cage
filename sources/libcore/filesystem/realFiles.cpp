#ifdef CAGE_SYSTEM_WINDOWS
	#include <vector> // wide characters
	#include "../windowsMinimumInclude.h"
	#include <io.h> // _get_osfhandle
	#define fseek64 _fseeki64
	#define ftell64 _ftelli64
	#define fseek invalidFunctionFseek
	#define ftell invalidFunctionFtell
#else
	#define _FILE_OFFSET_BITS 64
	#include <dirent.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#ifdef CAGE_SYSTEM_MAC
		#define fseek64 fseeko
		#define ftell64 ftello
		#define pread64 pread
		#define pwrite64 pwrite
		#include <mach-o/dyld.h>
	#else
		#define fseek64 fseeko64
		#define ftell64 ftello64
	#endif
#endif

#include "files.h"

#include <cage-core/math.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>

namespace cage
{
	Holder<File> realNewFile(const String &path, const FileMode &mode);
	PathTypeFlags realType(const String &path);

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		struct Widen
		{
			Widen(const String &path)
			{
				auto len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.size(), nullptr, 0);
				if (len <= 0)
					CAGE_THROW_ERROR(cage::SystemError, "MultiByteToWideChar", GetLastError());
				data.resize(len + 1);
				auto ret = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), path.size(), data.data(), data.size());
				CAGE_ASSERT(ret == len);
				data[ret] = 0;
			}

			operator const wchar_t *() const { return data.data(); }

			std::vector<wchar_t> data;
		};

		String narrow(PointerRange<const wchar_t> path)
		{
			String data;
			auto res = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, path.data(), path.size(), data.rawData(), String::MaxLength - 1, nullptr, nullptr);
			if (res == 0)
				CAGE_THROW_ERROR(cage::SystemError, "WideCharToMultiByte", GetLastError());
			data.rawData()[res] = 0;
			data.rawLength() = res;
			return data;
		}
#endif

		void realCreateDirectories(const String &path)
		{
			String pth = path + "/";
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
					const String p = subString(pth, 0, pos);
					if (any(realType(p) & PathTypeFlags::Directory))
						continue;

#ifdef CAGE_SYSTEM_WINDOWS
					if (CreateDirectoryW(Widen(p), nullptr) == 0)
					{
						const auto err = GetLastError();
						if (err != ERROR_ALREADY_EXISTS)
						{
							CAGE_LOG_THROW(Stringizer() + "path: " + path);
							CAGE_THROW_ERROR(SystemError, "CreateDirectory", err);
						}
					}
#else
					static constexpr mode_t mode = 0755;
					if (mkdir(p.c_str(), mode) != 0 && errno != EEXIST)
					{
						CAGE_LOG_THROW(Stringizer() + "path: " + path);
						CAGE_THROW_ERROR(Exception, "mkdir");
					}
#endif
				}
			}
		}

		class DirectoryListReal : public Immovable
		{
		public:
			const String myPath;
			bool valid_ = false;

#ifdef CAGE_SYSTEM_WINDOWS
			WIN32_FIND_DATAW ffd;
			HANDLE list = nullptr;
#else
			DIR *pdir = nullptr;
			struct dirent *pent = nullptr;
#endif

			DirectoryListReal(const String &path) : myPath(path)
			{
				realCreateDirectories(path);
#ifdef CAGE_SYSTEM_WINDOWS
				CAGE_ASSERT(!myPath.empty());
				list = FindFirstFileExW(Widen(myPath + "/*"), FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr, 0);
				if (list == INVALID_HANDLE_VALUE)
					CAGE_THROW_ERROR(SystemError, "FindFirstFileExW", GetLastError());
				valid_ = true;
				if (name() == "." || name() == "..")
					next();
#else
				pdir = opendir(path.c_str());
				valid_ = !!pdir;
				next();
#endif
			}

			~DirectoryListReal()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				if (list)
					FindClose(list);
#else
				if (pdir)
					closedir(pdir);
#endif
			}

			bool valid() const { return valid_; }

			String name() const
			{
#ifdef CAGE_SYSTEM_WINDOWS
				return narrow({ ffd.cFileName, ffd.cFileName + wcslen(ffd.cFileName) });
#else
				return pent->d_name;
#endif
			}

			void next()
			{
				CAGE_ASSERT(valid_);

#ifdef CAGE_SYSTEM_WINDOWS
				if (FindNextFileW(list, &ffd) == 0)
				{
					valid_ = false;
					return;
				}
#else
				pent = readdir(pdir);
				if (!pent)
				{
					valid_ = false;
					return;
				}
#endif

				if (name() == "." || name() == "..")
					next();
			}
		};

		void readAtImpl(PointerRange<char> buffer, const uint64 at, FILE *f)
		{
			CAGE_ASSERT(f);
#ifdef CAGE_SYSTEM_WINDOWS
			const HANDLE handle = (HANDLE)_get_osfhandle(_fileno(f));
			uint64 off = 0;
			uint64 s = buffer.size();
			while (s > 0)
			{
				OVERLAPPED o;
				detail::memset(&o, 0, sizeof(o));
				const uint64 p = at + off;
				o.Offset = (DWORD)p;
				o.OffsetHigh = (DWORD)((uint64)p >> 32);
				const uint64 k = min(s, (uint64)1024 * 1024 * 1024); // read in chunks of 1 GB
				DWORD r = 0;
				if (!ReadFile(handle, buffer.data() + off, numeric_cast<DWORD>(k), &r, &o) || r != k)
					CAGE_THROW_ERROR(SystemError, "ReadFile", GetLastError());
				off += k;
				s -= k;
			}
#else
			const int handle = fileno(f);
			uint64 off = 0;
			uint64 s = buffer.size();
			while (s > 0)
			{
				const uint64 k = min(s, (uint64)1024 * 1024 * 1024); // read in chunks of 1 GB
				if (pread64(handle, buffer.data() + off, k, at + off) != k)
					CAGE_THROW_ERROR(SystemError, "pread", errno);
				off += k;
				s -= k;
			}
#endif
		}

		void writeAtImpl(PointerRange<const char> buffer, const uint64 at, FILE *f)
		{
			CAGE_ASSERT(f);
#ifdef CAGE_SYSTEM_WINDOWS
			const HANDLE handle = (HANDLE)_get_osfhandle(_fileno(f));
			uint64 off = 0;
			uint64 s = buffer.size();
			while (s > 0)
			{
				OVERLAPPED o;
				detail::memset(&o, 0, sizeof(o));
				const uint64 p = at + off;
				o.Offset = (DWORD)p;
				o.OffsetHigh = (DWORD)((uint64)p >> 32);
				const uint64 k = min(s, (uint64)1024 * 1024 * 1024); // write in chunks of 1 GB
				DWORD r = 0;
				if (!WriteFile(handle, buffer.data() + off, numeric_cast<DWORD>(k), &r, &o) || r != k)
					CAGE_THROW_ERROR(SystemError, "WriteFile", GetLastError());
				off += k;
				s -= k;
			}
#else
			const int handle = fileno(f);
			uint64 off = 0;
			uint64 s = buffer.size();
			while (s > 0)
			{
				const uint64 k = min(s, (uint64)1024 * 1024 * 1024); // write in chunks of 1 GB
				if (pwrite64(handle, buffer.data() + off, k, at + off) != k)
					CAGE_THROW_ERROR(SystemError, "pwrite", errno);
				off += k;
				s -= k;
			}
#endif
		}

		class FileRealBase : public FileAbstract
		{
		public:
			FILE *f = nullptr;

			FileRealBase(const String &path, const FileMode &mode) : FileAbstract(path, mode)
			{
				CAGE_ASSERT(mode.valid());
				if (mode.write)
					realCreateDirectories(pathJoin(path, ".."));
#ifdef CAGE_SYSTEM_WINDOWS
				f = _wfopen(Widen(path), Widen(mode.mode()));
#else
				f = fopen(path.c_str(), mode.mode().c_str());
#endif
				if (!f)
				{
					CAGE_LOG_THROW(Stringizer() + "read: " + mode.read + ", write: " + mode.write + ", append: " + mode.append + ", text: " + mode.textual);
					CAGE_LOG_THROW(Stringizer() + "path: " + path);
					CAGE_THROW_ERROR(SystemError, "fopen", errno);
				}
			}

			~FileRealBase()
			{
				if (f)
				{
					try
					{
						close();
					}
					catch (const cage::Exception &)
					{
						// do nothing
					}
				}
			}

			void readAt(PointerRange<char> buffer, uint64 at) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(myMode.read);
				if (buffer.size() == 0)
					return;
				readAtImpl(buffer, at, f);
			}

			void read(PointerRange<char> buffer) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(myMode.read);
				if (buffer.size() == 0)
					return;
				if (fread(buffer.data(), buffer.size(), 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fread", errno);
			}

			void write(PointerRange<const char> buffer) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(myMode.write);
				if (buffer.size() == 0)
					return;
				if (fwrite(buffer.data(), buffer.size(), 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fwrite", errno);
			}

			void seek(uint64 position) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(position <= size());
				if (fseek64(f, position, 0) != 0)
					CAGE_THROW_ERROR(SystemError, "fseek", errno);
			}

			void close() override
			{
				CAGE_ASSERT(f);
				FILE *t = f;
				f = nullptr;
				if (fclose(t) != 0)
					CAGE_THROW_ERROR(SystemError, "fclose", errno);
			}

			uint64 tell() override
			{
				CAGE_ASSERT(f);
				return ftell64(f);
			}

			uint64 size() override
			{
				CAGE_ASSERT(f);
				const uint64 pos = ftell64(f);
				fseek64(f, 0, 2);
				const uint64 siz = ftell64(f);
				fseek64(f, pos, 0);
				return siz;
			}

			virtual void flush() = 0;
		};

		// binary files performs locking on all operations except actual data transfers to/from the drive
		class FileRealBinary final : public FileRealBase
		{
		public:
			FileRealBinary(const String &path, const FileMode &mode) : FileRealBase(path, mode) { CAGE_ASSERT(!mode.textual); }

			void reopenForModification() override
			{
				ScopeLock lock(fsMutex());
				CAGE_ASSERT(myMode.read && !myMode.write);
				CAGE_ASSERT(f);
				myMode.write = true;
#ifdef CAGE_SYSTEM_WINDOWS
				f = _wfreopen(Widen(myPath), Widen(myMode.mode()), f);
#else
				f = freopen(myPath.c_str(), myMode.mode().c_str(), f);
#endif
				if (!f)
				{
					CAGE_LOG_THROW(Stringizer() + "path: " + myPath);
					CAGE_THROW_ERROR(SystemError, "freopen", errno);
				}
			}

			void readAt(PointerRange<char> buffer, uint64 at) override
			{
				if (buffer.size() == 0)
					return;
				FILE *ff = nullptr;
				{
					ScopeLock lock(fsMutex());
					CAGE_ASSERT(f);
					CAGE_ASSERT(myMode.read);
					ff = f;
				}
				readAtImpl(buffer, at, ff);
			}

			void read(PointerRange<char> buffer) override
			{
				if (buffer.size() == 0)
					return;
				FILE *ff = nullptr;
				uint64 at = 0;
				{
					ScopeLock lock(fsMutex());
					CAGE_ASSERT(f);
					CAGE_ASSERT(myMode.read);
					ff = f;
					at = ftell64(f);
					if (fseek64(f, at + buffer.size(), 0) != 0)
						CAGE_THROW_ERROR(SystemError, "fseek", errno);
				}
				readAtImpl(buffer, at, ff);
			}

			void write(PointerRange<const char> buffer) override
			{
				if (buffer.size() == 0)
					return;
				FILE *ff = nullptr;
				uint64 at = 0;
				{
					ScopeLock lock(fsMutex());
					CAGE_ASSERT(f);
					CAGE_ASSERT(myMode.write);
					ff = f;
					at = ftell64(f);
					if (fseek64(f, at + buffer.size(), 0) != 0)
						CAGE_THROW_ERROR(SystemError, "fseek", errno);
				}
				writeAtImpl(buffer, at, ff);
			}

			void seek(uint64 position) override
			{
				ScopeLock lock(fsMutex());
				FileRealBase::seek(position);
			}

			void close() override
			{
				ScopeLock lock(fsMutex());
				FileRealBase::close();
			}

			uint64 tell() override
			{
				ScopeLock lock(fsMutex());
				return FileRealBase::tell();
			}

			uint64 size() override
			{
				ScopeLock lock(fsMutex());
				return FileRealBase::size();
			}

			void flush() override
			{
				FILE *ff = nullptr;
				{
					ScopeLock lock(fsMutex());
					CAGE_ASSERT(f);
					ff = f;
				}
				fflush(ff);
			}
		};

		// textual file does not do any locking and does not allow reopening for modification or seeking
		class FileRealTextual final : public FileRealBase
		{
		public:
			FileRealTextual(const String &path, const FileMode &mode) : FileRealBase(path, mode) { CAGE_ASSERT(mode.textual); }

			void readAt(PointerRange<char> buffer, uint64 at) override { CAGE_THROW_ERROR(Exception, "reading with offset in textual file is not allowed"); }

			void read(PointerRange<char> buffer) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(myMode.read);
				if (buffer.size() == 0)
					return;
				if (fread(buffer.data(), buffer.size(), 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fread", errno);
			}

			void write(PointerRange<const char> buffer) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(myMode.write);
				if (buffer.size() == 0)
					return;
				if (fwrite(buffer.data(), buffer.size(), 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fwrite", errno);
			}

			void seek(uint64 position) override { CAGE_THROW_ERROR(Exception, "seeking in textual file is not allowed"); }

			void flush() override
			{
				CAGE_ASSERT(f);
				fflush(f);
			}
		};

		class ArchiveReal final : public ArchiveAbstract
		{
		public:
			ArchiveReal(const String &path) : ArchiveAbstract(path) {}

			PathTypeFlags type(const String &path) const override
			{
				ScopeLock lock(fsMutex());
				return realType(pathJoin(myPath, path));
			}

			void createDirectories(const String &path) override
			{
				ScopeLock lock(fsMutex());
				realCreateDirectories(pathJoin(myPath, path));
			}

			void moveImpl(const String &from_, const String &to_, bool copying)
			{
				const String from = pathJoin(myPath, from_);
				const String to = pathJoin(myPath, to_);
#ifdef CAGE_SYSTEM_WINDOWS
				auto res = copying ? CopyFileW(Widen(from), Widen(to), false) : MoveFileExW(Widen(from), Widen(to), MOVEFILE_REPLACE_EXISTING);
				if (res == 0)
				{
					CAGE_LOG_THROW(Stringizer() + "path from: " + from + ", to: " + to);
					CAGE_THROW_ERROR(SystemError, "CopyFileW/MoveFileW", GetLastError());
				}
#else
				if (copying)
				{
					Holder<PointerRange<char>> buff = openFile(from_, FileMode(true, false))->readAll();
					openFile(to_, FileMode(false, true))->write(buff);
				}
				else
				{
					auto res = rename(from.c_str(), to.c_str());
					if (res != 0)
					{
						CAGE_LOG_THROW(Stringizer() + "path from: " + from + ", to: " + to);
						CAGE_THROW_ERROR(SystemError, "rename", errno);
					}
				}
#endif
			}

			void move(const String &from_, const String &to_, bool copying) override
			{
				if (from_ == to_)
					return;
				ScopeLock lock(fsMutex());
				createDirectories(pathJoin(to_, ".."));
				switch (type(from_))
				{
					case PathTypeFlags::NotFound:
					{
						CAGE_LOG_THROW(Stringizer() + "path: " + from_);
						CAGE_THROW_ERROR(Exception, "path to move/copy does not exist");
						break;
					}
					case PathTypeFlags::File:
					{
						moveImpl(from_, to_, copying);
						break;
					}
					case PathTypeFlags::Directory:
					{
						if (!copying && type(to_) == PathTypeFlags::NotFound)
							moveImpl(from_, to_, false);
						else
						{
							const auto list = listDirectory(from_);
							for (const String &it : list)
							{
								const String n = pathExtractFilename(it);
								move(pathJoin(from_, n), pathJoin(to_, n), copying);
							}
						}
						break;
					}
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid path type flags");
				}
			}

			void remove(const String &path_) override
			{
				ScopeLock lock(fsMutex());
				const String path = pathJoin(myPath, path_);
				const PathTypeFlags t = type(path_);
				if (any(t & PathTypeFlags::Directory))
				{
					{
						DirectoryListReal list(path);
						while (list.valid())
						{
							remove(pathJoin(path_, list.name()));
							list.next();
						}
					}

#ifdef CAGE_SYSTEM_WINDOWS
					if (RemoveDirectoryW(Widen(path)) == 0)
						CAGE_THROW_ERROR(SystemError, "RemoveDirectory", GetLastError());
#else
					if (rmdir(path.c_str()) != 0)
						CAGE_THROW_ERROR(SystemError, "rmdir", errno);
#endif
				}
				else if (none(t & PathTypeFlags::NotFound))
				{
#ifdef CAGE_SYSTEM_WINDOWS
					if (DeleteFileW(Widen(path)) == 0)
						CAGE_THROW_ERROR(SystemError, "DeleteFile", GetLastError());
#else
					if (unlink(path.c_str()) != 0)
						CAGE_THROW_ERROR(SystemError, "unlink", errno);
#endif
				}
			}

			PathLastChange lastChange(const String &path_) const override
			{
				ScopeLock lock(fsMutex());
				const String path = pathJoin(myPath, path_);
#ifdef CAGE_SYSTEM_WINDOWS
				HANDLE hFile = CreateFileW(Widen(path), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
				if (hFile == INVALID_HANDLE_VALUE)
				{
					CAGE_LOG_THROW(Stringizer() + "path: " + path);
					CAGE_THROW_ERROR(Exception, "failed to retrieve file last modification time");
				}
				FILETIME ftWrite;
				if (!GetFileTime(hFile, nullptr, nullptr, &ftWrite))
				{
					CloseHandle(hFile);
					CAGE_LOG_THROW(Stringizer() + "path: " + path);
					CAGE_THROW_ERROR(Exception, "failed to retrieve file last modification time");
				}
				ULARGE_INTEGER l;
				l.LowPart = ftWrite.dwLowDateTime;
				l.HighPart = ftWrite.dwHighDateTime;
				CloseHandle(hFile);
				return PathLastChange{ numeric_cast<uint64>(l.QuadPart) };
#else
				struct stat st;
				if (stat(pathToAbs(path).c_str(), &st) == 0)
					return PathLastChange{ numeric_cast<uint64>(st.st_mtime) };
				CAGE_LOG_THROW(Stringizer() + "path: " + path);
				CAGE_THROW_ERROR(SystemError, "stat", errno);
#endif
			}

			Holder<File> openFile(const String &path, const FileMode &mode) override
			{
				ScopeLock lock(fsMutex());
				return realNewFile(pathJoin(myPath, path), mode);
			}

			Holder<PointerRange<String>> listDirectory(const String &path) const override
			{
				ScopeLock lock(fsMutex());
				PointerRangeHolder<String> res;
				DirectoryListReal list(pathJoin(myPath, path));
				while (list.valid())
				{
					res.push_back(pathJoin(list.myPath, list.name()));
					list.next();
				}
				return res;
			}
		};

		String pathWorkingDirImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			wchar_t buffer[String::MaxLength + 1];
			uint32 len = GetCurrentDirectoryW(String::MaxLength, buffer);
			if (len <= 0)
				CAGE_THROW_ERROR(SystemError, "GetCurrentDirectory", GetLastError());
			return pathSimplify(narrow({ buffer, buffer + len }));
#else
			char buffer[String::MaxLength + 1];
			if (getcwd(buffer, String::MaxLength) != nullptr)
				return pathSimplify(buffer);
			CAGE_THROW_ERROR(Exception, "getcwd");
#endif
		}

		String executableFullPathImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			wchar_t buffer[String::MaxLength];
			uint32 len = GetModuleFileNameW(nullptr, (wchar_t *)&buffer, String::MaxLength);
			if (len == 0)
				CAGE_THROW_ERROR(SystemError, "GetModuleFileName", GetLastError());
			return pathSimplify(narrow({ buffer, buffer + len }));
#elif defined(CAGE_SYSTEM_LINUX)
			char buffer[String::MaxLength];
			char id[String::MaxLength];
			sprintf(id, "/proc/%d/exe", getpid());
			sint32 len = readlink(id, buffer, String::MaxLength);
			if (len == -1)
				CAGE_THROW_ERROR(SystemError, "readlink", errno);
			return pathSimplify(String({ buffer, buffer + len }));
#elif defined(CAGE_SYSTEM_MAC)
			char buffer[String::MaxLength];
			uint32 len = sizeof(buffer);
			if (_NSGetExecutablePath(buffer, &len) != 0)
				CAGE_THROW_ERROR(Exception, "_NSGetExecutablePath");
			len = std::strlen(buffer);
			return pathSimplify(String({ buffer, buffer + len }));
#else
	#error This operating system is not supported
#endif
		}
	}

	Holder<File> realNewFile(const String &path, const FileMode &mode)
	{
		return mode.textual ? systemMemory().createImpl<File, FileRealTextual>(path, mode) : systemMemory().createImpl<File, FileRealBinary>(path, mode);
	}

	PathTypeFlags realType(const String &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS
		auto a = GetFileAttributesW(Widen(path));
		if (a == INVALID_FILE_ATTRIBUTES)
			return PathTypeFlags::NotFound;
		if ((a & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			return PathTypeFlags::Directory;
		else
			return PathTypeFlags::File;
#else
		struct stat st;
		if (stat(path.c_str(), &st) != 0)
			return PathTypeFlags::NotFound;
		if ((st.st_mode & S_IFDIR) == S_IFDIR)
			return PathTypeFlags::Directory;
		if ((st.st_mode & S_IFREG) == S_IFREG)
			return PathTypeFlags::File;
		return PathTypeFlags::None;
#endif
	}

	void realTryFlushFile(File *f_)
	{
		if (FileRealBase *f = dynamic_cast<FileRealBase *>(f_))
			f->flush();
	}

	std::shared_ptr<ArchiveAbstract> archiveOpenReal(const String &path)
	{
		return std::make_shared<ArchiveReal>(path);
	}

	String pathWorkingDir()
	{
		static const String dir = pathWorkingDirImpl();
		CAGE_ASSERT(dir == pathWorkingDirImpl());
		return dir;
	}

	namespace detail
	{
		String executableFullPath()
		{
			static const String pth = executableFullPathImpl();
			return pth;
		}

		String executableFullPathNoExe()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			String p = executableFullPath();
			CAGE_ASSERT(isPattern(toLower(p), "", "", ".exe"));
			return subString(p, 0, p.length() - 4);
#else
			return executableFullPath();
#endif
		}
	}
}
