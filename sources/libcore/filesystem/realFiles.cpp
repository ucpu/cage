#include <cage-core/string.h>
#include <cage-core/timer.h>
#include <cage-core/concurrent.h> // threadSleep

#include "files.h"

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#define fseek _fseeki64
#define ftell _ftelli64
#else
#define _FILE_OFFSET_BITS 64
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#endif
#ifdef CAGE_SYSTEM_MAC
#include <mach-o/dyld.h>
#endif

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <set>

#include <FileWatcher/FileWatcher.h>

namespace cage
{
	PathTypeFlags realType(const string &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		auto a = GetFileAttributes(path.c_str());
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

	void realCreateDirectories(const string &path)
	{
		string pth = path + "/";
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
				if (any(realType(p) & PathTypeFlags::Directory))
					continue;

#ifdef CAGE_SYSTEM_WINDOWS
				if (CreateDirectory(p.c_str(), nullptr) == 0)
				{
					const auto err = GetLastError();
					if (err != ERROR_ALREADY_EXISTS)
					{
						CAGE_LOG_THROW(stringizer() + "path: '" + path + "'");
						CAGE_THROW_ERROR(SystemError, "CreateDirectory", err);
					}
				}
#else
				constexpr mode_t mode = 0755;
				if (mkdir(p.c_str(), mode) != 0 && errno != EEXIST)
				{
					CAGE_LOG_THROW(stringizer() + "path: '" + path + "'");
					CAGE_THROW_ERROR(Exception, "mkdir");
				}
#endif
			}
		}
	}

	void realMove(const string &from, const string &to)
	{
		pathCreateDirectories(pathExtractDirectory(to));

#ifdef CAGE_SYSTEM_WINDOWS

		auto res = MoveFile(from.c_str(), to.c_str());
		if (res == 0)
		{
			CAGE_LOG_THROW(stringizer() + "path from: '" + from + "'" + ", to: '" + to + "'");
			CAGE_THROW_ERROR(SystemError, "pathMove", GetLastError());
		}

#else

		auto res = rename(from.c_str(), to.c_str());
		if (res != 0)
		{
			CAGE_LOG_THROW(stringizer() + "path from: '" + from + "'" + ", to: '" + to + "'");
			CAGE_THROW_ERROR(SystemError, "pathMove", errno);
		}

#endif
	}

	void realRemove(const string &path)
	{
		const PathTypeFlags t = realType(path);
		if (any(t & PathTypeFlags::Directory))
		{
			Holder<DirectoryList> list = newDirectoryList(path);
			while (list->valid())
			{
				pathRemove(pathJoin(path, list->name()));
				list->next();
			}
#ifdef CAGE_SYSTEM_WINDOWS
			if (RemoveDirectory(path.c_str()) == 0)
				CAGE_THROW_ERROR(SystemError, "RemoveDirectory", GetLastError());
#else
			if (rmdir(path.c_str()) != 0)
				CAGE_THROW_ERROR(SystemError, "rmdir", errno);
#endif
		}
		else if (none(t & PathTypeFlags::NotFound))
		{
#ifdef CAGE_SYSTEM_WINDOWS
			if (DeleteFile(path.c_str()) == 0)
				CAGE_THROW_ERROR(SystemError, "DeleteFile", GetLastError());
#else
			if (unlink(path.c_str()) != 0)
				CAGE_THROW_ERROR(SystemError, "unlink", errno);
#endif
		}
	}

	uint64 realLastChange(const string &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			CAGE_LOG_THROW(stringizer() + "path: '" + path + "'");
			CAGE_THROW_ERROR(Exception, "path does not exist");
		}
		FILETIME ftWrite;
		if (!GetFileTime(hFile, nullptr, nullptr, &ftWrite))
		{
			CAGE_LOG_THROW(stringizer() + "path: '" + path + "'");
			CAGE_THROW_ERROR(Exception, "path does not exist");
		}
		ULARGE_INTEGER l;
		l.LowPart = ftWrite.dwLowDateTime;
		l.HighPart = ftWrite.dwHighDateTime;
		CloseHandle(hFile);
		return l.QuadPart;

#else

		struct stat st;
		if (stat(pathToAbs(path).c_str(), &st) == 0)
			return st.st_mtime;
		CAGE_LOG_THROW(stringizer() + "path: '" + path + "'");
		CAGE_THROW_ERROR(SystemError, "stat", errno);

#endif
	}

	namespace
	{
		string pathWorkingDirImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS

			char buffer[string::MaxLength];
			uint32 len = GetCurrentDirectory(string::MaxLength - 1, buffer);
			if (len <= 0)
				CAGE_THROW_ERROR(SystemError, "GetCurrentDirectory", GetLastError());
			if (len >= string::MaxLength)
				CAGE_THROW_ERROR(Exception, "path too long");
			return pathSimplify(string({ buffer, buffer + len }));

#else

			char buffer[string::MaxLength];
			if (getcwd(buffer, string::MaxLength - 1) != nullptr)
				return pathSimplify(buffer);
			CAGE_THROW_ERROR(Exception, "getcwd");

#endif
		}
	}

	string pathWorkingDir()
	{
		static const string dir = pathWorkingDirImpl();
		CAGE_ASSERT(dir == pathWorkingDirImpl());
		return dir;
	}

	namespace
	{
		string getExecutableFullPathImpl()
		{
			char buffer[string::MaxLength];

#ifdef CAGE_SYSTEM_WINDOWS

			uint32 len = GetModuleFileName(nullptr, (char *)&buffer, string::MaxLength);
			if (len == 0)
				CAGE_THROW_ERROR(SystemError, "GetModuleFileName", GetLastError());

#elif defined(CAGE_SYSTEM_LINUX)

			char id[string::MaxLength];
			sprintf(id, "/proc/%d/exe", getpid());
			sint32 len = readlink(id, buffer, string::MaxLength);
			if (len == -1)
				CAGE_THROW_ERROR(SystemError, "readlink", errno);

#elif defined(CAGE_SYSTEM_MAC)

			uint32 len = sizeof(buffer);
			if (_NSGetExecutablePath(buffer, &len) != 0)
				CAGE_THROW_ERROR(Exception, "_NSGetExecutablePath");
			len = detail::strlen(buffer);

#else

#error This operating system is not supported

#endif

			return pathSimplify(string({ buffer, buffer + len }));
		}
	}

	namespace detail
	{
		string getExecutableFullPath()
		{
			static const string pth = getExecutableFullPathImpl();
			return pth;
		}

		string getExecutableFullPathNoExe()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			string p = getExecutableFullPath();
			CAGE_ASSERT(isPattern(toLower(p), "", "", ".exe"));
			return subString(p, 0, p.length() - 4);
#else
			return getExecutableFullPath();
#endif
		}
	}

	namespace
	{
		class FileReal : public FileAbstract
		{
		public:
			FILE *f = nullptr;

			FileReal(const string &path, const FileMode &mode) : FileAbstract(path, mode)
			{
				CAGE_ASSERT(mode.valid());
				if (mode.write)
					realCreateDirectories(pathJoin(path, ".."));
				f = fopen(path.c_str(), mode.mode().c_str());
				if (!f)
				{
					CAGE_LOG_THROW(stringizer() + "read: " + mode.read + ", write: " + mode.write + ", append: " + mode.append + ", text: " + mode.textual);
					CAGE_LOG_THROW(stringizer() + "path: " + path);
					CAGE_THROW_ERROR(SystemError, "fopen", errno);
				}
			}

			~FileReal()
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

			void reopenForModification() override
			{
				CAGE_ASSERT(mode.read && !mode.write);
				CAGE_ASSERT(f);
				mode.write = true;
				f = freopen(myPath.c_str(), mode.mode().c_str(), f);
				if (!f)
				{
					CAGE_LOG_THROW(stringizer() + "path: " + myPath);
					CAGE_THROW_ERROR(SystemError, "freopen", errno);
				}
			}

			void read(PointerRange<char> buffer) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(mode.read);
				if (buffer.size() == 0)
					return;
				if (fread(buffer.data(), buffer.size(), 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fread", errno);
			}

			void write(PointerRange<const char> buffer) override
			{
				CAGE_ASSERT(f);
				CAGE_ASSERT(mode.write);
				if (buffer.size() == 0)
					return;
				if (fwrite(buffer.data(), buffer.size(), 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fwrite", errno);
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(f);
				if (fseek(f, position, 0) != 0)
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

			uintPtr tell() const override
			{
				CAGE_ASSERT(f);
				return numeric_cast<uintPtr>(ftell(f));
			}

			uintPtr size() const override
			{
				CAGE_ASSERT(f);
				auto pos = ftell(f);
				fseek(f, 0, 2);
				auto siz = ftell(f);
				fseek(f, pos, 0);
				return numeric_cast<uintPtr>(siz);
			}
		};
	}

	Holder<File> realNewFile(const string &path, const FileMode &mode)
	{
		return detail::systemArena().createImpl<File, FileReal>(path, mode);
	}

	namespace
	{
		class DirectoryListReal : public DirectoryListAbstract
		{
		public:
			bool valid_ = false;

#ifdef CAGE_SYSTEM_WINDOWS
			WIN32_FIND_DATA ffd;
			HANDLE list = nullptr;
#else
			DIR *pdir = nullptr;
			struct dirent *pent = nullptr;
#endif

			DirectoryListReal(const string &path) : DirectoryListAbstract(path)
			{
				realCreateDirectories(path);

#ifdef CAGE_SYSTEM_WINDOWS
				list = FindFirstFile(pathJoin(myPath, "*").c_str(), &ffd);
				valid_ = list != INVALID_HANDLE_VALUE;
				if (!valid_)
					return;
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

			bool valid() const override
			{
				return valid_;
			}

			string name() const override
			{
#ifdef CAGE_SYSTEM_WINDOWS
				return ffd.cFileName;
#else
				return pent->d_name;
#endif
			}

			void next() override
			{
				CAGE_ASSERT(valid_);

#ifdef CAGE_SYSTEM_WINDOWS

				if (FindNextFile(list, &ffd) == 0)
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
	}

	Holder<DirectoryList> realNewDirectoryList(const string &path)
	{
		return detail::systemArena().createImpl<DirectoryList, DirectoryListReal>(path);
	}

	namespace
	{
		class FilesystemWatcherImpl : public FilesystemWatcher, public FW::FileWatchListener
		{
		public:
			std::set<string, StringComparatorFast> files;
			Holder<FW::FileWatcher> fw;
			Holder<Timer> clock;

			FilesystemWatcherImpl()
			{
				fw = detail::systemArena().createHolder<FW::FileWatcher>();
				clock = newTimer();
			}

			const string waitForChange(uint64 time)
			{
				clock->reset();
				while (files.empty())
				{
					fw->update();
					if (files.empty() && clock->microsSinceStart() > time)
						return "";
					else
						threadSleep(1000 * 100);
				}
				const string res = *files.begin();
				files.erase(files.begin());
				return res;
			}

			virtual void handleFileAction(FW::WatchID watchid, const FW::String &dir, const FW::String &filename, FW::Action action) override
			{
				files.insert(pathJoin(dir.c_str(), filename.c_str()));
			}
		};
	}

	void FilesystemWatcher::registerPath(const string &path)
	{
		FilesystemWatcherImpl *impl = (FilesystemWatcherImpl *)this;
		impl->fw->addWatch(path.c_str(), impl);
		Holder<DirectoryList> dl = newDirectoryList(path);
		while (dl->valid())
		{
			const auto type = dl->type();
			if (any(type & PathTypeFlags::Directory) && none(type & (PathTypeFlags::Archive | PathTypeFlags::InsideArchive)))
				registerPath(pathJoin(path, dl->name()));
			dl->next();
		}
	}

	string FilesystemWatcher::waitForChange(uint64 time)
	{
		FilesystemWatcherImpl *impl = (FilesystemWatcherImpl *)this;
		return impl->waitForChange(time);
	}

	Holder<FilesystemWatcher> newFilesystemWatcher()
	{
		return detail::systemArena().createImpl<FilesystemWatcher, FilesystemWatcherImpl>();
	}
}
