#include "filesystem.h"

#include <cage-core/memoryBuffer.h>
#include <cage-core/math.h> // min

namespace cage
{
	fileVirtual::fileVirtual(const string &path, const fileMode &mode) : myPath(path), mode(mode)
	{}

	bool fileMode::valid() const
	{
		if (!read && !write)
			return false;
		if (append && !write)
			return false;
		if (textual && read)
			return false; // forbid reading files in text mode, we will do the line-ending conversions on our own
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
		uint64 pos = tell();
		uint64 siz = size();
		uint64 left = siz - pos;
		if (left == 0)
			return false;
		uint32 s = numeric_cast<uint32>(min(left, (uint64)string::MaxLength));
		char *lineData = const_cast<char*>(line.c_str());
		read(lineData, s);
		line = string(lineData, s);
		auto p = line.find('\n');
		if (p == m)
		{
			if (s == string::MaxLength)
				CAGE_THROW_ERROR(exception, "line too long");
			p = s;
			seek(siz);
		}
		else
			seek(pos + p + 1);
		if (p == 0)
		{
			line = "";
			return true;
		}
		line = string(lineData, p);
		if (!line.empty() && line[line.length() - 1] == '\r')
			line = line.subString(0, line.length() - 1);
		return true;
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
