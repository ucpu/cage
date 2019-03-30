#include <vector>
#include <initializer_list>

#include "filesystem.h"

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
			return path.replace("\\", "/");
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
		return path.find(':') != -1 || (path.length() > 0 && path[0] == '/');
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

	string pathJoin(const string &a, const string &b)
	{
		if (b.empty())
			return a;
		if (pathIsAbs(b))
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "first path: '" + a + "'");
			CAGE_LOG(severityEnum::Note, "exception", string() + "second path: '" + b + "'");
			CAGE_THROW_ERROR(exception, "cannot join with absolute path on right side");
		}
		if (a.empty())
			return b;
		string result = pathSimplify(a + "/" + b);
		CAGE_ASSERT_RUNTIME(pathIsAbs(result) == pathIsAbs(a), a, b, result, pathIsAbs(result), pathIsAbs(a));
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
				res += string(&tmp[i], 1);
			else
				res += replacement;
		}
		return res;
	}

	void pathDecompose(const string &input, string &drive, string &directory, string &file, string &extension)
	{
		// find drive
		string p = normalize(input);
		uint32 i = p.find(':');
		if (i == -1)
			drive = "";
		else
		{
			drive = p.subString(0, i);
			p = p.subString(i + 1, -1);
			if (p.empty() || p[0] != '/')
				p = string("/") + p;
		}
		// find filename
		p = p.reverse();
		i = p.find('/');
		if (i == -1)
		{
			file = p;
			directory = "";
		}
		else
		{
			file = p.subString(0, i);
			directory = p.subString(i, -1).reverse();
			if (directory.length() > 1)
				directory = directory.subString(0, directory.length() - 1);
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
		i = file.find('.');
		if (i == -1)
		{
			extension = "";
			file = file.reverse();
		}
		else
		{
			extension = file.subString(0, i + 1).reverse();
			file = file.subString(i + 1, -1).reverse();
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

	pathTypeFlags pathType(const string &path)
	{
		if (!pathIsValid(path))
			return pathTypeFlags::Invalid;
		string p;
		auto a = archiveFindTowardsRoot(path, true, p);
		if (a)
		{
			if (p.empty())
				return pathTypeFlags::File | pathTypeFlags::Archive;
			return a->type(p) | pathTypeFlags::InsideArchive;
		}
		else
			return realType(path);
	}

	bool pathIsFile(const string &path)
	{
		return (pathType(path) & pathTypeFlags::File) == pathTypeFlags::File;
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
		if (af == at)
			return af->move(pf, pt);
		mixedMove(af, pf, at, pt);
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

	string pathSearchTowardsRoot(const string &name, pathTypeFlags type)
	{
		return pathSearchTowardsRoot(name, pathWorkingDir(), type);
	}

	string pathSearchTowardsRoot(const string &name, const string &whereToStart, pathTypeFlags type)
	{
		if (name.empty())
			CAGE_THROW_ERROR(exception, "name cannot be empty");
		try
		{
			detail::overrideException oe;
			string p = whereToStart;
			while (true)
			{
				string s = pathJoin(p, name);
				if ((pathType(s) & type) != pathTypeFlags::None)
					return s;
				p = pathJoin(p, "..");
			}
		}
		catch (const exception &)
		{
			CAGE_LOG(severityEnum::Note, "exception", string() + "name: '" + name + "'");
			CAGE_LOG(severityEnum::Note, "exception", string() + "whereToStart: '" + whereToStart + "'");
			CAGE_THROW_ERROR(exception, "failed to find the path");
		}
	}
}
