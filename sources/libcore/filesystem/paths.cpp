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
			case '\\':
				return false;
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

		bool validateCharacters(const String &name, bool allowSlash)
		{
			for (const char c : name)
				if (!validateCharacter(c, allowSlash))
					return false;
			return true;
		}

		String normalize(const String &path)
		{
			return replace(path, "\\", "/");
		}

		String joinDrive(const String &d, const String &p)
		{
			if (d.empty())
				return p;
			if (p.empty())
				return d + ":/";
			CAGE_ASSERT(p[0] == '/');
			return d + ":" + p;
		}

		bool simplifyImplNoThrow(String &s)
		{
			const bool absolute = !s.empty() && s[0] == '/';
			std::vector<String> parts;
			while (!s.empty())
			{
				const String p = split(s, "/");
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
						return false;
				}
				parts.push_back(p);
			}
			CAGE_ASSERT(s.empty());
			for (const String &p : parts)
				s = pathJoinUnchecked(s, p);
			if (absolute)
				s = String() + "/" + s;
			return true;
		}

		bool decomposeImplNoThrow(const String &input, String &drive, String &directory, String &file, String &extension)
		{
			CAGE_ASSERT(drive.empty());
			CAGE_ASSERT(directory.empty());
			CAGE_ASSERT(file.empty());
			CAGE_ASSERT(extension.empty());

			String s = normalize(input);

			{ // separate drive
				const uint32 colon = find(s, ":/");
				const uint32 slash = find(s, '/');
				if (colon != m && slash == colon + 1)
				{
					if (colon == 0)
						return false;
					drive = subString(s, 0, colon);
					if (!validateCharacters(drive, false))
						return false;
					s = subString(s, colon + 1, m); // path starts with slash
				}
			}

			if (!validateCharacters(s, true))
				return false;

			if (!simplifyImplNoThrow(s))
				return false;

			{ // separate directory
				const bool absolute = !s.empty() && s[0] == '/';
				const uint32 j = find(reverse(s), '/');
				if (j != m)
				{
					const uint32 k = s.length() - j - 1;
					directory = subString(s, 0, k);
					s = subString(s, k + 1, m);
				}
				if (absolute && directory.empty())
					directory = "/";
				if (s == "..")
				{
					directory = pathJoinUnchecked(directory, s);
					return true;
				}
			}

			{ // separate file & extension
				const uint32 k = s.length() - find(reverse(s), '.') - 1;
				file = subString(s, 0, k);
				extension = subString(s, k, m);
			}

			return true;
		}
	}

	bool pathIsValid(const String &path)
	{
		String d, p, f, e;
		return decomposeImplNoThrow(path, d, p, f, e);
	}

	bool pathIsAbs(const String &path)
	{
		String d, p, f, e;
		pathDecompose(path, d, p, f, e);
		return !p.empty() && p[0] == '/';
	}

	String pathToRel(const String &path, const String &ref)
	{
		if (!pathIsAbs(path))
			return pathSimplify(path);
		String r = pathToAbs(ref);
		String p = pathSimplify(path);
		while (!r.empty() || !p.empty())
		{
			String r2 = r;
			String p2 = p;
			String r1 = split(r, "/");
			String p1 = split(p, "/");
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
			p = String("../") + p;
		}
		return pathSimplify(p);
	}

	String pathToAbs(const String &path)
	{
		String d, p, f, e;
		pathDecompose(path, d, p, f, e);
		if (d + p + f + e == "")
			return pathWorkingDir();

		if (!p.empty() && p[0] == '/')
		{
#ifdef CAGE_SYSTEM_WINDOWS
			// windows may have multiple roots, we want to be specific
			if (d.empty())
				d = pathExtractDrive(pathWorkingDir());
#endif // CAGE_SYSTEM_WINDOWS
			return joinDrive(d, pathJoinUnchecked(p, f + e));
		}

		if (!d.empty())
		{
#ifdef CAGE_SYSTEM_WINDOWS
			// it is ok on windows, if the protocol is same as the drive for working directory
			if (d != pathExtractDrive(pathWorkingDir()))
#endif // CAGE_SYSTEM_WINDOWS
			{
				CAGE_LOG_THROW(Stringizer() + "path: '" + path + "'");
				CAGE_THROW_ERROR(Exception, "path with protocol cannot be made absolute");
			}
		}

		return pathJoin(pathWorkingDir(), path);
	}

	String pathJoin(const String &a, const String &b)
	{
		String bd, bp, bf, be;
		pathDecompose(b, bd, bp, bf, be);
		if (!bd.empty() || (!bp.empty() && bp[0] == '/'))
		{
			CAGE_LOG_THROW(Stringizer() + "first path: '" + a + "'");
			CAGE_LOG_THROW(Stringizer() + "second path: '" + b + "'");
			CAGE_THROW_ERROR(Exception, "cannot join with absolute path");
		}
		bp = pathJoinUnchecked(bp, bf + be);
		String ad, ap, af, ae;
		pathDecompose(a, ad, ap, af, ae);
		ap = pathJoinUnchecked(ap, af + ae);
		String r = pathJoinUnchecked(ap, bp);
		if (!simplifyImplNoThrow(r))
		{
			CAGE_LOG_THROW(Stringizer() + "first path: '" + a + "'");
			CAGE_LOG_THROW(Stringizer() + "second path: '" + b + "'");
			CAGE_THROW_ERROR(Exception, "cannot join paths that would go beyond root");
		}
		if (ad.empty() && find(r, ":/") < find(r, '/'))
			r = String() + "./" + r;
		return joinDrive(ad, r);
	}

	String pathJoinUnchecked(const String &a, const String &b)
	{
		if (a.empty())
			return b;
		if (b.empty())
			return a;
		if (a[a.length() - 1] == '/')
			return a + b;
		return a + "/" + b;
	}

	String pathSimplify(const String &path)
	{
		String d, p, f, e;
		pathDecompose(path, d, p, f, e);
		return joinDrive(d, pathJoinUnchecked(p, f + e));
	}

	String pathReplaceInvalidCharacters(const String &path, const String &replacement, bool allowDirectories)
	{
		CAGE_ASSERT(validateCharacters(replacement, false));
		String res;
		for (char c : normalize(path))
		{
			if (validateCharacter(c, allowDirectories))
				res += String(c);
			else
				res += replacement;
		}
		return res;
	}

	void pathDecompose(const String &input, String &drive, String &directory, String &file, String &extension)
	{
		if (!decomposeImplNoThrow(input, drive, directory, file, extension))
		{
			CAGE_LOG_THROW(Stringizer() + "path: '" + input + "'");
			CAGE_THROW_ERROR(Exception, "invalid path");
		}
	}

	String pathExtractDrive(const String &input)
	{
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return d;
	}

	String pathExtractDirectory(const String &input)
	{
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return joinDrive(d, p);
	}

	String pathExtractDirectoryNoDrive(const String &input)
	{
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return p;
	}

	String pathExtractFilename(const String &input)
	{
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return f + e;
	}

	String pathExtractFilenameNoExtension(const String &input)
	{
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return f;
	}

	String pathExtractExtension(const String &input)
	{
		String d, p, f, e;
		pathDecompose(input, d, p, f, e);
		return e;
	}
}
