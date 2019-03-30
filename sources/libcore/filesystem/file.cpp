#include "filesystem.h"

#include <cage-core/memoryBuffer.h>

namespace cage
{
	fileVirtual::fileVirtual(const string &path, const fileMode &mode) : myPath(path), mode(mode)
	{}

	bool fileMode::valid() const
	{
		if (append && !write)
			return false;
		if (!read && !write)
			return false;
		return true;
	}

	string fileMode::mode() const
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

	void fileClass::read(void *data, uint64 size)
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->read(data, size);
	}

	bool fileClass::readLine(string &line)
	{
		fileVirtual *impl = (fileVirtual *)this;
		line = "";
		uint64 left = size() - tell();
		if (left == 0)
			return false;
		char c = 0;
		while (left--)
		{
			read(&c, 1);
			if (c == '\n')
			{
#ifdef CAGE_SYSTEM_WINDOWS
				if (!line.empty() && line[line.length() - 1] == '\r')
					line = line.subString(0, line.length() - 1);
#endif
				return true;
			}
			line += string(&c, 1);
		}
		return !line.empty();
	}

	memoryBuffer fileClass::readBuffer(uintPtr size)
	{
		memoryBuffer r(size);
		read(r.data(), r.size());
		return r;
	}

	void fileClass::write(const void *data, uint64 size)
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->write(data, size);
	}

	void fileClass::writeLine(const string &data)
	{
		string d = data + "\n";
		write(d.c_str(), d.length());
	}

	void fileClass::writeBuffer(const memoryBuffer &buffer)
	{
		write(buffer.data(), buffer.size());
	}

	void fileClass::seek(uint64 position)
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->seek(position);
	}

	void fileClass::flush()
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->flush();
	}

	void fileClass::close()
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->close();
	}

	uint64 fileClass::tell() const
	{
		fileVirtual *impl = (fileVirtual *)this;
		return impl->tell();
	}

	uint64 fileClass::size() const
	{
		fileVirtual *impl = (fileVirtual *)this;
		return impl->size();
	}

	holder<fileClass> newFile(const string &path, const fileMode &mode)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, false, p);
		if (a)
			return a->file(p, mode);
		else
			return realNewFile(path, mode);
	}
}
