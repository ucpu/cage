#include "filesystem.h"
#include <cage-core/debug.h>
#include <cage-core/string.h>

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
#include <vector>
#include <initializer_list>

namespace cage
{
	namespace
	{
		bool validateCharacter(char c, bool allowSlash)
		{
#ifdef CAGE_SYSTEM_WINDOWS
			switch (c)
			{
			case '<':
			case '>':
			case ':':
			case '"':
			case '|':
			case '?':
			case '*':
				return false;
			case '\\':
			case '/':
				return allowSlash;
			default:
				return c >= 32;
			}
#else
			switch (c)
			{
			case '/':
				return allowSlash;
			default:
				return c >= 32;
			}
#endif
		}

		string normalize(const string &path)
		{
			return replace(path, "\\", "/");
		}
	}

	bool pathIsValid(const string &path)
	{
		string d, p, f, e;
		pathDecompose(path, d, p, f, e);
		uint32 i = 0;
		for (const string &it : { d, p, f, e })
		{
			for (auto c : it)
				if (!validateCharacter(c, i == 1))
					return false;
			i++;
		}
		return true;
	}

	bool pathIsAbs(const string &path)
	{
		return find(path, ':') != m || (path.length() > 0 && path[0] == '/');
	}

	string pathToRel(const string &path, const string &ref)
	{
		if (!pathIsAbs(path))
			return pathSimplify(path);
		string r = pathToAbs(ref);
		string p = pathSimplify(path);
		while (!r.empty() || !p.empty())
		{
			string r2 = r;
			string p2 = p;
			string r1 = split(r, "/");
			string p1 = split(p, "/");
			if (r1 == p1)
				continue;
			r = r2;
			p = p2;
			break;
		}
		if (pathIsAbs(p))
			return pathSimplify(p);
		while (!r.empty())
		{
			split(r, "/");
			p = string("../") + p;
		}
		return pathSimplify(p);
	}

	string pathToAbs(const string &path)
	{
		if (path.empty())
			return pathWorkingDir();
		if (pathIsAbs(path))
			return pathSimplify(path);
		return pathJoin(pathWorkingDir(), path);
	}

	string pathJoin(const string &a, const string &b)
	{
		if (b.empty())
			return a;
		if (pathIsAbs(b))
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "first path: '" + a + "'");
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "second path: '" + b + "'");
			CAGE_THROW_ERROR(Exception, "cannot join with absolute path on right side");
		}
		if (a.empty())
			return b;
		string result = pathSimplify(a + "/" + b);
		CAGE_ASSERT(pathIsAbs(result) == pathIsAbs(a));
		return result;
	}

	string pathSimplify(const string &path)
	{
		string drive, directory, file, extension;
		pathDecompose(path, drive, directory, file, extension);
		bool absolute = !drive.empty() || (!directory.empty() && directory[0] == '/');
		std::vector<string> parts;
		while (true)
		{
			if (directory.empty())
				break;
			string p = split(directory, "/");
			if (p == "" || p == ".")
				continue;
			if (p == "..")
			{
				if (!parts.empty() && parts[parts.size() - 1] != "..")
				{
					parts.pop_back();
					continue;
				}
				if (absolute)
					CAGE_THROW_ERROR(Exception, "path cannot go beyond root");
			}
			parts.push_back(p);
		}
		string result;
		if (!drive.empty())
			result += drive + ":/";
		else if (absolute)
			result += "/";
		directory = "";
		for (auto it : parts)
		{
			if (!directory.empty())
				directory += "/";
			directory += it;
		}
		result += directory;
		file += extension;
		if (!result.empty() && !file.empty() && result[result.length() - 1] != '/')
			result += string() + "/" + file;
		else
			result += file;
		return result;
	}

	string pathReplaceInvalidCharacters(const string &path, const string &replacement, bool allowDirectories)
	{
		string tmp = normalize(path);
		string res;
		for (uint32 i = 0, e = tmp.length(); i < e; i++)
		{
			if (validateCharacter(tmp[i], allowDirectories))
				res += string({ &tmp[i], &tmp[i] + 1 });
			else
				res += replacement;
		}
		return res;
	}

	void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension)
	{
		// find drive
		string p = normalize(input);
		uint32 i = find(p, ':');
		if (i == m)
			drive = "";
		else
		{
			drive = subString(p, 0, i);
			p = subString(p, i + 1, m);
			if (p.empty() || p[0] != '/')
				p = string("/") + p;
		}
		// find filename
		p = reverse(p);
		i = find(p, '/');
		if (i == m)
		{
			file = p;
			directory = "";
		}
		else
		{
			file = subString(p, 0, i);
			directory = reverse(subString(p, i, m));
			if (directory.length() > 1)
				directory = subString(directory, 0, directory.length() - 1);
		}
		if (file == "." || file == "..")
		{
			if (directory.empty())
				directory = file;
			else if (directory[directory.length() - 1] == '/')
				directory += file;
			else
				directory += string("/") + file;
			file = "";
		}
		// find extension
		i = find(file,'.');
		if (i == m)
		{
			extension = "";
			file = reverse(file);
		}
		else
		{
			extension = reverse(subString(file, 0, i + 1));
			file = reverse(subString(file, i + 1, m));
		}
	}

	string pathExtractDrive(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return d;
	}

	string pathExtractPath(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		p = pathSimplify(p);
		if (d.empty())
			return p;
		return d + ":/" + p;
	}

	string pathExtractPathNoDrive(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return pathSimplify(p);
	}

	string pathExtractFilename(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return f + e;
	}

	string pathExtractFilenameNoExtension(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return f;
	}

	string pathExtractExtension(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return e;
	}

	PathTypeFlags pathType(const string &path)
	{
		if (!pathIsValid(path))
			return PathTypeFlags::Invalid;
		string p;
		auto a = archiveFindTowardsRoot(path, true, p);
		if (a)
		{
			if (p.empty())
				return PathTypeFlags::File | PathTypeFlags::Archive;
			return a->type(p) | PathTypeFlags::InsideArchive;
		}
		else
			return realType(path);
	}

	bool pathIsFile(const string &path)
	{
		return (pathType(path) & PathTypeFlags::File) == PathTypeFlags::File;
	}

	void pathCreateDirectories(const string &path)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, true, p);
		if (a)
			a->createDirectories(p);
		else
			realCreateDirectories(path);
	}

	void pathMove(const string &from, const string &to)
	{
		string pf, pt;
		auto af = archiveFindTowardsRoot(from, false, pf);
		auto at = archiveFindTowardsRoot(to, false, pt);
		if (!af && !at)
			return realMove(from, to);
		if (af && af == at)
			return af->move(pf, pt);
		mixedMove(af, af ? pf : from, at, at ? pt : to);
	}

	void pathRemove(const string &path)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, false, p);
		if (a)
			a->remove(p);
		else
			realRemove(path);
	}

	uint64 pathLastChange(const string &path)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, false, p);
		if (a)
			return a->lastChange(p);
		else
			return realLastChange(path);
	}

	string pathSearchTowardsRoot(const string &name, PathTypeFlags type)
	{
		return pathSearchTowardsRoot(name, pathWorkingDir(), type);
	}

	string pathSearchTowardsRoot(const string &name, const string &whereToStart, PathTypeFlags type)
	{
		if (name.empty())
			CAGE_THROW_ERROR(Exception, "name cannot be empty");
		try
		{
			detail::OverrideException oe;
			string p = whereToStart;
			while (true)
			{
				string s = pathJoin(p, name);
				if ((pathType(s) & type) != PathTypeFlags::None)
					return s;
				p = pathJoin(p, "..");
			}
		}
		catch (const Exception &)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "name: '" + name + "'");
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "whereToStart: '" + whereToStart + "'");
			CAGE_THROW_ERROR(Exception, "failed to find the path");
		}
	}

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
				if ((realType(p) & PathTypeFlags::Directory) == PathTypeFlags::Directory)
					continue;

#ifdef CAGE_SYSTEM_WINDOWS
				if (CreateDirectory(p.c_str(), nullptr) == 0)
				{
					const auto err = GetLastError();
					if (err != ERROR_ALREADY_EXISTS)
					{
						CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
						CAGE_THROW_ERROR(SystemError, "CreateDirectory", err);
					}
				}
#else
				constexpr mode_t mode = 0755;
				if (mkdir(p.c_str(), mode) != 0 && errno != EEXIST)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
					CAGE_THROW_ERROR(Exception, "mkdir");
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
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path from: '" + from + "'" + ", to: '" + to + "'");
			CAGE_THROW_ERROR(SystemError, "pathMove", GetLastError());
		}

#else

		auto res = rename(from.c_str(), to.c_str());
		if (res != 0)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path from: '" + from + "'" + ", to: '" + to + "'");
			CAGE_THROW_ERROR(SystemError, "pathMove", errno);
		}

#endif
	}

	void realRemove(const string &path)
	{
		PathTypeFlags t = realType(path);
		if ((t & PathTypeFlags::Directory) == PathTypeFlags::Directory)
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
		else if ((t & PathTypeFlags::NotFound) == PathTypeFlags::None)
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
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
			CAGE_THROW_ERROR(Exception, "path does not exist");
		}
		FILETIME ftWrite;
		if (!GetFileTime(hFile, nullptr, nullptr, &ftWrite))
		{
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
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
		CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + path + "'");
		CAGE_THROW_ERROR(SystemError, "stat", errno);

#endif
	}

	string pathWorkingDir()
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
			static string pth = getExecutableFullPathImpl();
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
}
