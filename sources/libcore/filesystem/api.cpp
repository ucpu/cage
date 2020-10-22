#include <cage-core/memoryBuffer.h>
#include <cage-core/lineReader.h>
#include <cage-core/math.h> // min
#include <cage-core/debug.h>

#include "files.h"

namespace cage
{
	bool FileMode::valid() const
	{
		if (!read && !write)
			return false;
		if (append && !write)
			return false;
		if (textual && read)
			return false; // forbid reading files in text mode, we will do the line-ending conversions on our own
		return true;
	}

	string FileMode::mode() const
	{
		string md;
		if (read && !write)
			md = "r";
		else if (read && write)
		{
			if (append)
				md = "a+";
			else
				md = "w+";
		}
		else if (!read && write)
		{
			if (append)
				md = "a";
			else
				md = "w";
		}
		md += textual ? "t" : "b";
		return md;
	}

	void File::read(PointerRange<char> buffer)
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->read(buffer.data(), buffer.size());
	}

	MemoryBuffer File::read(uintPtr size)
	{
		MemoryBuffer r(size);
		read(r);
		return r;
	}

	MemoryBuffer File::readAll()
	{
		CAGE_ASSERT(tell() == 0);
		MemoryBuffer r(size());
		read(r);
		return r;
	}

	bool File::readLine(string &line)
	{
		FileAbstract *impl = (FileAbstract *)this;

		const uintPtr origPos = tell();
		const uintPtr origSize = size();
		const uintPtr origLeft = origSize - origPos;
		if (origLeft == 0)
			return false;

		char buffer[string::MaxLength + 1];
		PointerRange<char> pr1 = { buffer, buffer + numeric_cast<uint32>(min(origLeft, (uintPtr)string::MaxLength)) };
		read(pr1);
		PointerRange<const char> pr2 = pr1;
		if (!detail::readLine(line, pr2, origLeft >= string::MaxLength))
		{
			seek(origPos);
			if (origLeft >= string::MaxLength)
				CAGE_THROW_ERROR(Exception, "line too long");
			return false;
		}
		seek(min(origPos + (pr2.begin() - pr1.begin()), origSize));
		return true;
	}

	void File::write(PointerRange<const char> buffer)
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->write(buffer.data(), buffer.size());
	}

	void File::writeLine(const string &line)
	{
		const string d = line + "\n";
		write({ d.c_str(), d.c_str() + d.length() });
	}

	void File::seek(uintPtr position)
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->seek(position);
	}

	void File::close()
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->close();
	}

	uintPtr File::tell() const
	{
		const FileAbstract *impl = (const FileAbstract *)this;
		return impl->tell();
	}

	uintPtr File::size() const
	{
		const FileAbstract *impl = (const FileAbstract *)this;
		return impl->size();
	}

	FileMode File::mode() const
	{
		const FileAbstract *impl = (const FileAbstract *)this;
		return impl->mode;
	}

	Holder<File> newFile(const string &path, const FileMode &mode)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, false, p);
		if (a)
			return a->openFile(p, mode);
		else
			return realNewFile(path, mode);
	}

	Holder<File> readFile(const string &path)
	{
		return newFile(path, FileMode(true, false));
	}

	Holder<File> writeFile(const string &path)
	{
		return newFile(path, FileMode(false, true));
	}

	bool DirectoryList::valid() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return impl->valid();
	}

	string DirectoryList::name() const
	{
		const DirectoryListAbstract *impl = (const DirectoryListAbstract *)this;
		return impl->name();
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
			CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: '" + impl->fullPath() + "'");
			CAGE_THROW_ERROR(Exception, "path is not directory");
		}
		return newDirectoryList(impl->fullPath());
	}

	void DirectoryList::next()
	{
		DirectoryListAbstract *impl = (DirectoryListAbstract *)this;
		impl->next();
	}

	Holder<DirectoryList> newDirectoryList(const string &path)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, true, p);
		if (a)
			return a->listDirectory(p);
		else
			return realNewDirectoryList(path);
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
		return any(pathType(path) & PathTypeFlags::File);
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

	void pathCreateArchive(const string &path, const string &options)
	{
		// someday, switch based on the options may be implemented here to create different types of archives
		// the options are passed on to allow for other options (compression, encryption, ...)
		archiveCreateZip(path, options);
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
}
