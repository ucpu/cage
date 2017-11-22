#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/filesystem.h>
#include <cage-core/log.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <dirent.h>
#endif

namespace cage
{
	namespace
	{
		class directoryListImpl : public directoryListClass
		{
		public:
			string path;
			bool valid;

#ifdef CAGE_SYSTEM_WINDOWS
			WIN32_FIND_DATA ffd;
			HANDLE list;
#else
			DIR *pdir;
			struct dirent *pent;
#endif

			directoryListImpl(const string &path) : path(path), valid(false)
#ifdef CAGE_SYSTEM_WINDOWS
				, list(nullptr)
#else
				, pdir(nullptr), pent(nullptr)
#endif
			{
				pathCreateDirectories(path);

#ifdef CAGE_SYSTEM_WINDOWS

				HANDLE hFile = CreateFile(path.c_str(), 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					char buffer[string::MaxLength];
					DWORD len = GetFinalPathNameByHandle(hFile, buffer, string::MaxLength, VOLUME_NAME_DOS);
					CloseHandle(hFile);
					if (len < string::MaxLength)
						this->path = string(buffer + 4);
				}

				list = FindFirstFile(pathJoin(this->path, "*").c_str(), &ffd);
				valid = list != INVALID_HANDLE_VALUE;
				if (!valid)
					return;
				if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
					next();

#else

				pdir = opendir(path.c_str());
				valid = !!pdir;
				next();

#endif
			}

			~directoryListImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				if (list)
					FindClose(list);
#else
				if (pdir)
					closedir(pdir);
#endif
			}

			const string name() const
			{
#ifdef CAGE_SYSTEM_WINDOWS
				return ffd.cFileName;
#else
				return pent->d_name;
#endif
			}

			const string fullPath() const
			{
				return pathJoin(path, name());
			}
		};
	}

	void directoryListClass::next()
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->valid, "can not advance on invalid list");

#ifdef CAGE_SYSTEM_WINDOWS

		if (FindNextFile(impl->list, &impl->ffd) == 0)
		{
			impl->valid = false;
			return;
		}

#else

		impl->pent = readdir(impl->pdir);
		if (!impl->pent)
		{
			impl->valid = false;
			return;
		}

#endif

		if (name() == "." || name() == "..")
			next();
	}

	holder<fileClass> directoryListClass::openFile(const fileMode &mode)
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		return newFile(impl->fullPath(), mode);
	}

	holder<filesystemClass> directoryListClass::openDirectory()
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(exception, "is not dir");
		holder<filesystemClass> fs = newFilesystem();
		fs->changeDir(impl->fullPath());
		return fs;
	}

	holder<directoryListClass> directoryListClass::directoryList()
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		if (!isDirectory())
			CAGE_THROW_ERROR(exception, "is not dir");
		return newDirectoryList(impl->fullPath());
	}

	bool directoryListClass::valid() const
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		return impl->valid;
	}

	string directoryListClass::name() const
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		return impl->name();
	}

	bool directoryListClass::isDirectory() const
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		return pathIsDirectory(impl->fullPath());
	}

	uint64 directoryListClass::lastChange() const
	{
		directoryListImpl *impl = (directoryListImpl *)this;
		return pathLastChange(impl->fullPath());
	}

	holder<directoryListClass> newDirectoryList(const string &path)
	{
		return detail::systemArena().createImpl<directoryListClass, directoryListImpl>(path);
	}
}
