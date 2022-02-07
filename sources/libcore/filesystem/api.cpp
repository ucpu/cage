#include <cage-core/pointerRangeHolder.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/lineReader.h>
#include <cage-core/math.h> // min
#include <cage-core/debug.h>
#include <cage-core/string.h>

#include "files.h"

namespace cage
{
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
		CAGE_THROW_CRITICAL(NotImplemented, "reading from abstract (possibly write-only) file");
	}

	Holder<PointerRange<char>> File::read(uintPtr size)
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
		CAGE_THROW_CRITICAL(NotImplemented, "writing to abstract (possibly read-only) file");
	}

	void File::writeLine(const String &line)
	{
		write(line);
		write("\n");
	}

	void File::seek(uintPtr position)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "calling seek on an abstract file");
	}

	void File::close()
	{
		CAGE_THROW_CRITICAL(NotImplemented, "calling close on an abstract file");
	}

	uintPtr File::tell()
	{
		CAGE_THROW_CRITICAL(NotImplemented, "calling tell on an abstract file");
	}

	uintPtr File::size()
	{
		CAGE_THROW_CRITICAL(NotImplemented, "calling size on an abstract file");
	}

	FileMode File::mode() const
	{
		CAGE_THROW_CRITICAL(NotImplemented, "calling mode on an abstract file");
	}

	Holder<File> newFile(const String &path, const FileMode &mode)
	{
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::FileExclusiveThrow);
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

	bool DirectoryList::valid() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return impl->valid();
	}

	String DirectoryList::name() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return impl->name();
	}

	String DirectoryList::fullPath() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return impl->fullPath();
	}

	PathTypeFlags DirectoryList::type() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return pathType(impl->fullPath());
	}

	bool DirectoryList::isDirectory() const
	{
		return any(type() & (PathTypeFlags::Directory | PathTypeFlags::Archive));
	}

	uint64 DirectoryList::lastChange() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return pathLastChange(impl->fullPath());
	}

	Holder<File> DirectoryList::openFile(const FileMode &mode)
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		return newFile(impl->fullPath(), mode);
	}

	Holder<File> DirectoryList::readFile()
	{
		return openFile(FileMode(true, false));
	}

	Holder<File> DirectoryList::writeFile()
	{
		return openFile(FileMode(false, true));
	}

	Holder<DirectoryList> DirectoryList::listDirectory()
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		if (!isDirectory())
		{
			CAGE_LOG_THROW(Stringizer() + "path: '" + impl->fullPath() + "'");
			CAGE_THROW_ERROR(Exception, "path is not directory");
		}
		return newDirectoryList(impl->fullPath());
	}

	void DirectoryList::next()
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		impl->next();
	}

	Holder<DirectoryList> newDirectoryList(const String &path)
	{
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveShared);
		return a->listDirectory(p);
	}

	PathTypeFlags pathType(const String &path)
	{
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

	void pathCreateDirectories(const String &path)
	{
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveShared);
		a->createDirectories(p);
	}

	void pathCreateArchive(const String &path, const String &options)
	{
		if (any(pathType(path) & (PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)))
		{
			CAGE_LOG_THROW(Stringizer() + "path: '" + path + "'");
			CAGE_THROW_ERROR(Exception, "cannot create archive, the path already exists");
		}
		archiveCreateZip(path, options);
	}

	void pathMove(const String &from, const String &to)
	{
		auto [af, pf] = archiveFindTowardsRoot(from, ArchiveFindModeEnum::FileExclusiveThrow);
		auto [at, pt] = archiveFindTowardsRoot(to, ArchiveFindModeEnum::FileExclusiveThrow);
		if (af == at)
			return af->move(pf, pt);
		mixedMove(af, pf, at, pt);
	}

	void pathRemove(const String &path)
	{
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::FileExclusiveThrow);
		a->remove(p);
	}

	uint64 pathLastChange(const String &path)
	{
		auto [a, p] = archiveFindTowardsRoot(path, ArchiveFindModeEnum::ArchiveExclusiveThrow);
		return a->lastChange(p);
	}

	String pathSearchTowardsRoot(const String &name, PathTypeFlags type)
	{
		return pathSearchTowardsRoot(name, pathWorkingDir(), type);
	}

	String pathSearchTowardsRoot(const String &name, const String &whereToStart, PathTypeFlags type)
	{
		CAGE_ASSERT(any(type & (PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)));
		CAGE_ASSERT(none(type & ~(PathTypeFlags::File | PathTypeFlags::Directory | PathTypeFlags::Archive)));
		if (name.empty() || !pathIsValid(name) || pathIsAbs(name))
		{
			CAGE_LOG_THROW(Stringizer() + "name: '" + name + "'");
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
			CAGE_LOG_THROW(Stringizer() + "name: '" + name + "'");
			CAGE_LOG_THROW(Stringizer() + "whereToStart: '" + whereToStart + "'");
			CAGE_THROW_ERROR(Exception, "pathSearchTowardsRoot failed to find the name");
		}
	}

	Holder<PointerRange<String>> pathSearchSequence(const String &pattern, char substitute, uint32 limit)
	{
		PointerRangeHolder<String> result;
		const uint32 firstDollar = find(pattern, substitute);
		if (firstDollar == m)
		{
			if (pathIsFile(pattern))
				result.push_back(pattern);
		}
		else
		{
			const uint32 dollarsCount = [&]() {
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
}
