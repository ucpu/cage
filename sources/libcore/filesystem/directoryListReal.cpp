#include "filesystem.h"

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <dirent.h>
#endif

namespace cage
{
	namespace
	{
		class directoryListReal : public directoryListVirtual
		{
		public:
			bool valid_;

#ifdef CAGE_SYSTEM_WINDOWS
			WIN32_FIND_DATA ffd;
			HANDLE list;
#else
			DIR *pdir;
			struct dirent *pent;
#endif

			directoryListReal(const string &path) : directoryListVirtual(path), valid_(false)
#ifdef CAGE_SYSTEM_WINDOWS
				, list(nullptr)
#else
				, pdir(nullptr), pent(nullptr)
#endif
			{
				pathCreateDirectories(path);

#ifdef CAGE_SYSTEM_WINDOWS

				/*
				HANDLE hFile = CreateFile(path.c_str(), 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					char buffer[string::MaxLength];
					DWORD len = GetFinalPathNameByHandle(hFile, buffer, string::MaxLength, VOLUME_NAME_DOS);
					CloseHandle(hFile);
					if (len < string::MaxLength)
						const_cast<string&>(this->path) = string(buffer + 4);
				}
				*/

				list = FindFirstFile(pathJoin(this->path, "*").c_str(), &ffd);
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

			~directoryListReal()
			{
#ifdef CAGE_SYSTEM_WINDOWS
				if (list)
					FindClose(list);
#else
				if (pdir)
					closedir(pdir);
#endif
			}

			void next() override
			{
				CAGE_ASSERT_RUNTIME(valid_, "can not advance on invalid list");

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
		};
	}

	holder<directoryListClass> realNewDirectoryList(const string &path)
	{
		return detail::systemArena().createImpl<directoryListClass, directoryListReal>(path);
	}
}
