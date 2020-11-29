#include <cage-core/string.h>

#include "files.h"

#include <vector>

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
		return find(path, ':') != m || (!path.empty() && path[0] == '/');
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
		{
#ifdef CAGE_SYSTEM_WINDOWS
			// windows may have multiple roots, we need to be specific
			string p = pathSimplify(path);
			if (p[0] == '/')
				return pathExtractDrive(pathWorkingDir()) + ":" + p;
			return p;
#else
			return pathSimplify(path);
#endif // CAGE_SYSTEM_WINDOWS
		}
		return pathJoin(pathWorkingDir(), path);
	}

	string pathJoin(const string &a, const string &b)
	{
		if (b.empty())
			return a;
		if (pathIsAbs(b))
		{
			CAGE_LOG_THROW(stringizer() + "first path: '" + a + "'");
			CAGE_LOG_THROW(stringizer() + "second path: '" + b + "'");
			CAGE_THROW_ERROR(Exception, "cannot join with absolute path on right side");
		}
		if (a.empty())
			return b;
		const string result = pathSimplify(a + "/" + b);
		CAGE_ASSERT(pathIsAbs(result) == pathIsAbs(a));
		return result;
	}

	string pathSimplify(const string &path)
	{
		string drive, directory, file, extension;
		pathDecompose(path, drive, directory, file, extension);
		const bool absolute = !drive.empty() || (!directory.empty() && directory[0] == '/');
		std::vector<string> parts;
		while (true)
		{
			if (directory.empty())
				break;
			const string p = split(directory, "/");
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
		for (const string &it : parts)
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

	namespace
	{
		bool validateCharacters(const string &name)
		{
			for (const char c : name)
				if (!validateCharacter(c, false))
					return false;
			return true;
		}
	}

	string pathReplaceInvalidCharacters(const string &path, const string &replacement, bool allowDirectories)
	{
		CAGE_ASSERT(validateCharacters(replacement));
		const string tmp = normalize(path);
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
		i = find(file, '.');
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

	string pathExtractDirectory(const string &input)
	{
		string d, p, f, e;
		pathDecompose(input, d, p, f, e);
		p = pathSimplify(p);
		if (d.empty())
			return p;
		return d + ":/" + p;
	}

	string pathExtractDirectoryNoDrive(const string &input)
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
}
