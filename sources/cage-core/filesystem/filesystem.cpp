#include <vector>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/filesystem.h>
#include <cage-core/log.h>
#include "../system.h"

namespace cage
{
	namespace
	{
		const bool validateCharacter(char c, bool allowSlash)
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
				return true;
			}
#endif
		}

		const string normalize(const string &path)
		{
			return path.replace("\\", "/");
		}

		class filesystemImpl : public filesystemClass
		{
		public:
			string current;

			const string makePath(const string &path) const
			{
				if (pathIsAbs(path))
					return pathToRel(path);
				else
					return pathToRel(pathJoin(current, path));
			}
		};
	}

	void filesystemClass::changeDir(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		impl->current = impl->makePath(path);
	}

	holder<directoryListClass> filesystemClass::directoryList(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return newDirectoryList(impl->makePath(path));
	}

	holder<changeWatcherClass> filesystemClass::changeWatcher(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		holder<changeWatcherClass> cw = newChangeWatcher();
		cw->registerPath(impl->makePath(path));
		return cw;
	}

	holder<fileClass> filesystemClass::openFile(const string &path, const fileMode &mode)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return newFile(impl->makePath(path), mode);
	}

	void filesystemClass::move(const string &from, const string &to)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathMove(impl->makePath(from), impl->makePath(to));
	}

	void filesystemClass::remove(const string &path)
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathRemove(impl->makePath(path));
	}

	string filesystemClass::currentDir() const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return impl->current;
	}

	bool filesystemClass::exists(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathExists(impl->makePath(path));
	}

	bool filesystemClass::isDirectory(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathIsDirectory(impl->makePath(path));
	}

	uint64 filesystemClass::lastChange(const string &path) const
	{
		filesystemImpl *impl = (filesystemImpl*)this;
		return pathLastChange(impl->makePath(path));
	}

	holder<filesystemClass> newFilesystem()
	{
		return detail::systemArena().createImpl<filesystemClass, filesystemImpl>();
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

	bool pathIsValid(const string &path)
	{
		for (uint32 i = 0, e = path.length(); i < e; i++)
			if (!validateCharacter(path[i], true))
				return false;
		return true;
	}

	string pathToRel(const string &path, const string &ref)
	{
		if (path.empty())
			return "";
		string r = pathToAbs(ref);
		string p = pathToAbs(path);
		while (!r.empty() || !p.empty())
		{
			string r2 = r;
			string p2 = p;
			string r1 = r.split("/");
			string p1 = p.split("/");
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
			r.split("/");
			p = string("../") + p;
		}
		return pathSimplify(p);
	}

	string pathToAbs(const string &path)
	{
		if (path.empty())
			return pathWorkingDir();
		if (pathIsAbs(path))
			return path;
		return pathJoin(pathWorkingDir(), path);
	}

	bool pathIsAbs(const string &path)
	{
		return path.find(':') != -1 || (path.length() > 0 && path[0] == '/');
	}

	void pathCreateDirectories(const string &path)
	{
		string pth = path + "/";
		uint32 off = 0;
		while (true)
		{
			uint32 pos = pth.subString(off, -1).find('/');
			if (pos == -1)
				return; // done
			pos += off;
			off = pos + 1;
			if (pos)
			{
				const string p = pth.subString(0, pos);
				if (pathIsDirectory(p))
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

	bool pathExists(const string &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		auto res = GetFileAttributes(path.c_str());
		return res != INVALID_FILE_ATTRIBUTES;

#else

		struct stat st;
		if (stat(path.c_str(), &st) == 0)
			return (st.st_mode & S_IFREG) == S_IFREG;
		return false;

#endif
	}

	bool pathIsDirectory(const string &path)
	{
#ifdef CAGE_SYSTEM_WINDOWS

		auto res = GetFileAttributes(path.c_str());
		if (res != INVALID_FILE_ATTRIBUTES)
			return !!(res & FILE_ATTRIBUTE_DIRECTORY);
		return false;

#else

		struct stat st;
		if (stat(path.c_str(), &st) == 0)
			return (st.st_mode & S_IFDIR) == S_IFDIR;
		return false;

#endif
	}

	void pathMove(const string &from, const string &to)
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

		CAGE_THROW_CRITICAL(notImplementedException, "pathMove");

#endif
	}

	void pathRemove(const string &path)
	{
		if (pathIsDirectory(path))
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
		else if (pathExists(path))
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

	string pathJoin(const string &a, const string &b)
	{
		if (a.empty())
			return b;
		if (b.empty())
			return a;
		if (pathIsAbs(b))
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "first path: '" + a + "'");
			CAGE_LOG(severityEnum::Note, "exception", string() + "second path: '" + b + "'");
			CAGE_THROW_ERROR(exception, "cannot join two absolute paths");
		}
		string result = pathSimplify(a + "/" + b);
		CAGE_ASSERT_RUNTIME(pathIsAbs(result) == pathIsAbs(a), a, b, result, pathIsAbs(result), pathIsAbs(a));
		return result;
	}

	uint64 pathLastChange(const string &path)
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

	string pathFind(const string &name)
	{
		if (name.empty() || pathIsAbs(name))
			return name;
		try
		{
			string p = pathWorkingDir();
			while (true)
			{
				string s = pathJoin(p, name);
				if (pathIsDirectory(s))
					return s;
				p = pathJoin(p, "..");
			}
		}
		catch (const exception &)
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + name + "'");
			CAGE_THROW_ERROR(exception, "failed to find the path");
		}
	}

	string pathSimplify(const string &path)
	{
		string drive, directory, file, extension;
		pathDecompose(path, drive, directory, file, extension);
		bool absolute = !drive.empty() || (!path.empty() && path[0] == '/');
		std::vector<string> parts;
		while (true)
		{
			if (directory.empty())
				break;
			string p = directory.split("/");
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
					CAGE_THROW_ERROR(exception, "path cannot go beyond root");
			}
			parts.push_back(p);
		}
		string result;
		if (!drive.empty())
			result += drive;
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

	string pathMakeValid(const string &path, const string &replacement, bool allowDirectories)
	{
		string tmp = normalize(path);
		string res;
		for (uint32 i = 0, e = tmp.length(); i < e; i++)
		{
			if (validateCharacter(tmp[i], allowDirectories))
				res += string(&tmp[i], 1);
			else
				res += replacement;
		}
		return res;
	}

	void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension)
	{
		string p = normalize(input);
		uint32 i = p.find(":/", 0);
		if (i != -1)
		{
			drive = p.subString(0, i) + "://";
			p = p.subString(i + 2, -1);
			if (!p.empty() && p[0] == '/')
				p = p.subString(1, -1);
		}
		else
			drive = "";
		p = p.reverse();
		i = p.find('/', 0);
		if (i != -1)
		{
			directory = p.subString(i + 1, -1).reverse();
			p = p.subString(0, i);
		}
		else
			directory = "";
		if (p == "." || p == "..")
		{
			if (!directory.empty() && directory[directory.length() - 1] != '/')
				directory += "/";
			directory += p;
			p = "";
		}
		i = p.find('.', 0);
		if (i != -1)
		{
			extension = string() + "." + p.subString(0, i).reverse();
			file = p.subString(i + 1, -1).reverse();
		}
		else
		{
			extension = "";
			file = p.reverse();
		}
	};

	string pathExtractDrive(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return d;
	};

	string pathExtractPath(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return d + p;
	};

	string pathExtractPathNoDrive(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return p;
	};

	string pathExtractFilename(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return f + e;
	};

	string pathExtractFilenameNoExtension(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return f;
	};

	string pathExtractExtension(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return e;
	};

	namespace detail
	{
		string getExecutableName()
		{
			char buffer[string::MaxLength];

#ifdef CAGE_SYSTEM_WINDOWS

			uint32 len = GetModuleFileName(nullptr, (char*)&buffer, string::MaxLength);
			if (len == 0)
				CAGE_THROW_ERROR(codeException, "GetModuleFileName", GetLastError());

#else

			char id[string::MaxLength];
			sprintf(id, "/proc/%d/exe", getpid());
			sint32 len = readlink(id, buffer, string::MaxLength);
			if (len == -1)
				CAGE_THROW_ERROR(codeException, "readlink", errno);

#endif

			return pathExtractFilename(string(buffer, len));
		}
	}
}
