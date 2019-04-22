#include "filesystem.h"

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

#ifdef CAGE_SYSTEM_MAC
#include <mach-o/dyld.h>
#endif

#include <cerrno>
#include <cstdio>

namespace cage
{
	pathTypeFlags realType(const string &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		auto a = GetFileAttributes(path.c_str());
		if (a == INVALID_FILE_ATTRIBUTES)
			return pathTypeFlags::NotFound;
		if ((a & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			return pathTypeFlags::Directory;
		else
			return pathTypeFlags::File;

#else

		struct stat st;
		if (stat(path.c_str(), &st) != 0)
			return pathTypeFlags::NotFound;
		if ((st.st_mode & S_IFDIR) == S_IFDIR)
			return pathTypeFlags::Directory;
		if ((st.st_mode & S_IFREG) == S_IFREG)
			return pathTypeFlags::File;
		return pathTypeFlags::None;

#endif
	}

	void realCreateDirectories(const string &path)
	{
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
				if ((realType(p) & pathTypeFlags::Directory) == pathTypeFlags::Directory)
					continue;

#ifdef CAGE_SYSTEM_WINDOWS
				if (CreateDirectory(p.c_str(), nullptr) == 0)
				{
					auto err = GetLastError();
					if (err != ERROR_ALREADY_EXISTS)
					{
						CAGE_LOG(severityEnum::Note, "exception", string() + "path: '" + path + "'");
						CAGE_THROW_ERROR(codeException, "CreateDirectory", err);
					}
				}
#else
				static const mode_t mode = 0755;
				if (mkdir(p.c_str(), mode) != 0 && errno != EEXIST)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "path: '" + path + "'");
					CAGE_THROW_ERROR(exception, "mkdir");
				}
#endif
			}
		}
	}

	void realMove(const string &from, const string &to)
	{
		pathCreateDirectories(pathExtractPath(to));

#ifdef CAGE_SYSTEM_WINDOWS

		auto res = MoveFile(from.c_str(), to.c_str());
		if (res == 0)
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "path from: '" + from + "'" + ", to: '" + to + "'");
			CAGE_THROW_ERROR(codeException, "pathMove", GetLastError());
		}

#else

		auto res = rename(from.c_str(), to.c_str());
		if (res != 0)
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "path from: '" + from + "'" + ", to: '" + to + "'");
			CAGE_THROW_ERROR(codeException, "pathMove", errno);
		}

#endif
	}

	void realRemove(const string &path)
	{
		pathTypeFlags t = realType(path);
		if ((t & pathTypeFlags::Directory) == pathTypeFlags::Directory)
		{
			holder<directoryListClass> list = newDirectoryList(path);
			while (list->valid())
			{
				pathRemove(pathJoin(path, list->name()));
				list->next();
			}
#ifdef CAGE_SYSTEM_WINDOWS
			if (RemoveDirectory(path.c_str()) == 0)
				CAGE_THROW_ERROR(codeException, "RemoveDirectory", GetLastError());
#else
			if (rmdir(path.c_str()) != 0)
				CAGE_THROW_ERROR(codeException, "rmdir", errno);
#endif
		}
		else if ((t & pathTypeFlags::NotFound) == pathTypeFlags::None)
		{
#ifdef CAGE_SYSTEM_WINDOWS
			if (DeleteFile(path.c_str()) == 0)
				CAGE_THROW_ERROR(codeException, "DeleteFile", GetLastError());
#else
			if (unlink(path.c_str()) != 0)
				CAGE_THROW_ERROR(codeException, "unlink", errno);
#endif
		}
	}

	uint64 realLastChange(const string &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "path: '" + path + "'");
			CAGE_THROW_ERROR(exception, "path does not exist");
		}
		FILETIME ftWrite;
		if (!GetFileTime(hFile, nullptr, nullptr, &ftWrite))
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "path: '" + path + "'");
			CAGE_THROW_ERROR(exception, "path does not exist");
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
		CAGE_LOG(severityEnum::Note, "exception", string() + "path: '" + path + "'");
		CAGE_THROW_ERROR(codeException, "stat", errno);

#endif
	}

	string pathWorkingDir()
	{
#ifdef CAGE_SYSTEM_WINDOWS

		char buffer[string::MaxLength];
		uint32 len = GetCurrentDirectory(string::MaxLength - 1, buffer);
		if (len <= 0)
			CAGE_THROW_ERROR(codeException, "GetCurrentDirectory", GetLastError());
		if (len >= string::MaxLength)
			CAGE_THROW_ERROR(exception, "path too long");
		return pathSimplify(string(buffer, len));

#else

		char buffer[string::MaxLength];
		if (getcwd(buffer, string::MaxLength - 1) != nullptr)
			return pathSimplify(buffer);
		CAGE_THROW_ERROR(exception, "getcwd");

#endif
	}

	namespace detail
	{
		string getExecutableFullPath()
		{
			char buffer[string::MaxLength];

#ifdef CAGE_SYSTEM_WINDOWS

			uint32 len = GetModuleFileName(nullptr, (char*)&buffer, string::MaxLength);
			if (len == 0)
				CAGE_THROW_ERROR(codeException, "GetModuleFileName", GetLastError());

#elif defined(CAGE_SYSTEM_LINUX)

			char id[string::MaxLength];
			sprintf(id, "/proc/%d/exe", getpid());
			sint32 len = readlink(id, buffer, string::MaxLength);
			if (len == -1)
				CAGE_THROW_ERROR(codeException, "readlink", errno);

#elif defined(CAGE_SYSTEM_MAC)

			uint32 len = sizeof(buffer);
			if (_NSGetExecutablePath(buffer, &len) != 0)
				CAGE_THROW_ERROR(exception, "_NSGetExecutablePath");
			len = detail::strlen(buffer);

#else

#error This operating system is not supported

#endif

			return pathSimplify(string(buffer, len));
		}

		string getExecutableFullPathNoExe()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			string p = getExecutableFullPath();
			CAGE_ASSERT_RUNTIME(p.isPattern("", "", ".exe"));
			return p.subString(0, p.length() - 4);
#else
			return getExecutableFullPath();
#endif
		}
	}
}
