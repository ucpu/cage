#include "files.h"

#include <cage-core/debug.h>
#include <cage-core/lineReader.h>
#include <cage-core/math.h> // min
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>

namespace cage
{
	void archiveCreateZip(const String &path);
	void archiveCreateCarch(const String &path);

	bool FileMode::valid() const
	{
		if (!read && !write)
			return false;
		if (append && !write)
			return false;
		if (append && read)
			return false; // forbid
		if (textual && read)
			return false; // forbid reading files in text mode, we will do the line-ending conversions on our own
		return true;
	}

	String FileMode::mode() const
	{
		CAGE_ASSERT(valid());
		String md;
		if (append)
			md = "a";
		else
		{
			if (read && write)
				md = "r+";
			else if (read)
				md = "r";
			else
				md = "w";
		}
		md += textual ? "t" : "b";
		return md;
	}

	void File::read(PointerRange<char> buffer)
	{
		CAGE_THROW_CRITICAL(Exception, "reading from abstract (possibly write-only) file");
	}

	Holder<PointerRange<char>> File::read(uint64 size)
	{
		MemoryBuffer r(size);
		read(r);
		return std::move(r);
	}

	Holder<PointerRange<char>> File::readAll()
	{
		const uintPtr s = size();
		if (!s)
			return {};
		MemoryBuffer r(s);
		read(r);
		return std::move(r);
	}

	String File::readLine()
	{
		String line;
		if (readLine(line))
			return line;
		return "";
	}

	bool File::readLine(String &line)
	{
		const uintPtr origPos = tell();
		const uintPtr origSize = size();
		const uintPtr origLeft = origSize - origPos;
		if (origLeft == 0)
			return false;

		char buffer[String::MaxLength + 1]; // plus 1 to allow detecting that the line is too long
		PointerRange<char> pr = { buffer, buffer + min(origLeft, sizeof(buffer)) };
		read(pr);
		try
		{
			uintPtr l = detail::readLine(line, pr, origLeft >= String::MaxLength);
			seek(origPos + l);
			return l;
		}
		catch (Exception &)
		{
			seek(origPos);
			throw;
		}
	}

	void File::write(PointerRange<const char> buffer)
	{
		CAGE_THROW_CRITICAL(Exception, "writing to abstract (possibly read-only) file");
	}

	void File::writeLine(const String &line)
	{
		write(line);
		write("\n");
	}

	void File::seek(uint64 position)
	{
		CAGE_THROW_CRITICAL(Exception, "calling seek on an abstract file");
	}

	void File::close()
	{
		CAGE_THROW_CRITICAL(Exception, "calling close on an abstract file");
	}

	uint64 File::tell()
	{
		CAGE_THROW_CRITICAL(Exception, "calling tell on an abstract file");
	}

	uint64 File::size()
	{
		CAGE_THROW_CRITICAL(Exception, "calling size on an abstract file");
	}

	FileMode File::mode() const
	{
		CAGE_THROW_CRITICAL(Exception, "calling mode on an abstract file");
	}

	Holder<File> newFile(const String &path, FileMode mode)
	{
		ScopeLock lock(fsMutex());
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::FileExclusive);
		return a->openFile(p, mode);
	}

	Holder<File> readFile(const String &path)
	{
		return newFile(path, FileMode(true, false));
	}

	Holder<File> writeFile(const String &path)
	{
		return newFile(path, FileMode(false, true));
	}

	PathTypeFlags pathType(const String &path)
	{
		ScopeLock lock(fsMutex());
		if (!pathIsValid(path))
			return PathTypeFlags::Invalid;
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveShared);
		if (p.empty())
			return PathTypeFlags::File | PathTypeFlags::Archive;
		return a->type(p);
	}

	bool pathIsFile(const String &path)
	{
		return any(pathType(path) & PathTypeFlags::File);
	}

	bool pathIsDirectory(const String &path)
	{
		return any(pathType(path) & (PathTypeFlags::Directory | PathTypeFlags::Archive));
	}

	PathLastChange pathLastChange(const String &path)
	{
		ScopeLock lock(fsMutex());
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveExclusive);
		return a->lastChange(p);
	}

	Holder<PointerRange<String>> pathListDirectory(const String &path)
	{
		ScopeLock lock(fsMutex());
		if (none(pathType(path) & (PathTypeFlags::Directory | PathTypeFlags::Archive)))
		{
			CAGE_LOG_THROW(Stringizer() + "path: " + path);
			CAGE_THROW_ERROR(Exception, "cannot list path that is not directory");
		}
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveShared);
		return a->listDirectory(p);
	}

	String pathSearchTowardsRoot(const String &name, PathTypeFlags type)
	{
		return pathSearchTowardsRoot(name, pathWorkingDir(), type);
	}

	String pathSearchTowardsRoot(const String &name, const String &whereToStart, PathTypeFlags type)
	{
		ScopeLock lock(fsMutex());
		CAGE_ASSERT(any(type & (PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)));
		CAGE_ASSERT(none(type & ~(PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)));
		if (name.empty() || !pathIsValid(name) || pathIsAbs(name))
		{
			CAGE_LOG_THROW(Stringizer() + "name: " + name);
			CAGE_THROW_ERROR(Exception, "invalid name in pathSearchTowardsRoot");
		}
		try
		{
			detail::OverrideException oe;
			String p = whereToStart;
			while (true)
			{
				const String s = pathJoin(p, name);
				if (any(pathType(s) & type))
					return s;
				p = pathJoin(p, "..");
			}
		}
		catch (const Exception &)
		{
			CAGE_LOG_THROW(Stringizer() + "name: " + name);
			CAGE_LOG_THROW(Stringizer() + "whereToStart: " + whereToStart);
			CAGE_THROW_ERROR(Exception, "pathSearchTowardsRoot failed to find the name");
		}
	}

	Holder<PointerRange<String>> pathSearchSequence(const String &pattern, char substitute, uint32 limit)
	{
		ScopeLock lock(fsMutex());
		PointerRangeHolder<String> result;
		const uint32 firstDollar = find(pattern, substitute);
		if (firstDollar == m)
		{
			if (pathIsFile(pattern))
				result.push_back(pattern);
		}
		else
		{
			const uint32 dollarsCount = [&]()
			{
				uint32 i = 0;
				String s = subString(pattern, firstDollar, m);
				while (i < s.length() && s[i] == substitute)
					i++;
				return i;
			}();
			const String prefix = subString(pattern, 0, firstDollar);
			const String suffix = subString(pattern, firstDollar + dollarsCount, m);
			for (uint32 i = 0; i < limit; i++)
			{
				const String name = prefix + reverse(fill(reverse(String(Stringizer() + i)), dollarsCount, '0')) + suffix;
				if (pathIsFile(name))
					result.push_back(name);
				else if (i > 0)
					break;
			}
		}
		return result;
	}

	void pathCreateDirectories(const String &path)
	{
		ScopeLock lock(fsMutex());
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveShared);
		a->createDirectories(p);
	}

	void pathCreateArchiveZip(const String &path)
	{
		ScopeLock lock(fsMutex());
		if (any(pathType(path) & (PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)))
		{
			CAGE_LOG_THROW(Stringizer() + "path: " + path);
			CAGE_THROW_ERROR(Exception, "cannot create archive, the path already exists");
		}
		archiveCreateZip(path);
	}

	void pathCreateArchiveCarch(const String &path)
	{
		ScopeLock lock(fsMutex());
		if (any(pathType(path) & (PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)))
		{
			CAGE_LOG_THROW(Stringizer() + "path: " + path);
			CAGE_THROW_ERROR(Exception, "cannot create archive, the path already exists");
		}
		archiveCreateCarch(path);
	}

	namespace
	{
		void copyFileContentMixed(const String &from, const String &to)
		{
			auto [af, pf] = archiveFindTowardsRoot(from, ArchiveFindModeEnum::FileExclusive);
			auto [at, pt] = archiveFindTowardsRoot(to, ArchiveFindModeEnum::FileExclusive);
			CAGE_ASSERT(af != at);
			Holder<PointerRange<char>> b = af->openFile(pf, FileMode(true, false))->readAll();
			at->openFile(pt, FileMode(false, true))->write(b);
		}

		void mergeRecursive(const String &from, const String &to, bool copying)
		{
			auto [af, pf] = archiveFindTowardsRoot(from, ArchiveFindModeEnum::ArchiveExclusive);
			auto [at, pt] = archiveFindTowardsRoot(to, ArchiveFindModeEnum::ArchiveExclusive);
			if (af == at)
				return af->move(pf, pt, copying);
			if (any(af->type(pf) & PathTypeFlags::File))
				copyFileContentMixed(from, to);
			else
			{
				const auto list = af->listDirectory(pf);
				for (const String &p : list)
				{
					const String n = pathExtractFilename(p);
					mergeRecursive(pathJoin(from, n), pathJoin(to, n), copying);
				}
			}
		}

		void moveImpl(const String &from_, const String &to_, bool copying)
		{
			const String from = pathToAbs(from_);
			const String to = pathToAbs(to_);
			if (from == to)
				return;
			if (subString(to + "/", 0, from.length()) == from + "/")
				CAGE_THROW_ERROR(Exception, "move/copy into itself");
			const PathTypeFlags fromFlags = pathType(from);
			if (fromFlags == PathTypeFlags::NotFound)
				CAGE_THROW_ERROR(Exception, "source for move/copy not found");
			const PathTypeFlags toFlags = pathType(to);
			if (fromFlags == PathTypeFlags::Directory && toFlags == PathTypeFlags::File)
				CAGE_THROW_ERROR(Exception, "cannot move/copy a directory onto a file");
			if (fromFlags == PathTypeFlags::File || toFlags == PathTypeFlags::File || toFlags == PathTypeFlags::NotFound)
			{ // replace
				auto [af, pf] = archiveFindTowardsRoot(from, ArchiveFindModeEnum::FileExclusive);
				auto [at, pt] = archiveFindTowardsRoot(to, ArchiveFindModeEnum::FileExclusive);
				if (af == at)
					af->move(pf, pt, copying);
				else if (any(fromFlags & PathTypeFlags::File))
					copyFileContentMixed(from, to);
				else
				{
					pathRemove(to);
					mergeRecursive(from, to, copying);
				}
			}
			else
			{ // merge
				mergeRecursive(from, to, copying);
			}
			if (!copying)
				pathRemove(from);
		}

		void moveImplEntry(const String &from, const String &to, bool copying)
		{
			try
			{
				moveImpl(from, to, copying);
			}
			catch (const cage::Exception &)
			{
				CAGE_LOG_THROW(Stringizer() + "from: " + from);
				CAGE_LOG_THROW(Stringizer() + "to: " + to);
				throw;
			}
		}
	}

	void pathMove(const String &from, const String &to)
	{
		ScopeLock lock(fsMutex());
		moveImplEntry(from, to, false);
	}

	void pathCopy(const String &from, const String &to)
	{
		ScopeLock lock(fsMutex());
		moveImplEntry(from, to, true);
	}

	void pathRemove(const String &path)
	{
		ScopeLock lock(fsMutex());
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::FileExclusive);
		a->remove(p);
	}

	namespace detail
	{
		Holder<void> pathKeepOpen(const String &path)
		{
			ScopeLock lock(fsMutex());
			auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveShared);
			if (p != "" && a->type(p) == PathTypeFlags::NotFound)
			{
				CAGE_LOG_THROW(Stringizer() + "path: " + path);
				CAGE_THROW_ERROR(Exception, "pathKeepOpen: path does not exist");
			}
			return systemMemory().createHolder<std::shared_ptr<ArchiveAbstract>>(std::move(a)).cast<void>();
		}
	}
}
