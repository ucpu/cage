#include "filesystem.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/math.h> // min

#ifdef CAGE_SYSTEM_WINDOWS

#include "../incWin.h"
#include <cstdio>
#define fseek _fseeki64
#define ftell _ftelli64

#else

#define _FILE_OFFSET_BITS 64
#include <cstdio>
#include <cerrno>

#endif

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

	void fileHandle::read(void *data, uint64 size)
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->read(data, size);
	}

	bool fileHandle::readLine(string &line)
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

	memoryBuffer fileHandle::readBuffer(uintPtr size)
	{
		memoryBuffer r(size);
		read(r.data(), r.size());
		return r;
	}

	void fileHandle::write(const void *data, uint64 size)
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->write(data, size);
	}

	void fileHandle::writeLine(const string &data)
	{
		string d = data + "\n";
		write(d.c_str(), d.length());
	}

	void fileHandle::writeBuffer(const memoryBuffer &buffer)
	{
		write(buffer.data(), buffer.size());
	}

	void fileHandle::seek(uint64 position)
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->seek(position);
	}

	void fileHandle::flush()
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->flush();
	}

	void fileHandle::close()
	{
		fileVirtual *impl = (fileVirtual *)this;
		impl->close();
	}

	uint64 fileHandle::tell() const
	{
		fileVirtual *impl = (fileVirtual *)this;
		return impl->tell();
	}

	uint64 fileHandle::size() const
	{
		fileVirtual *impl = (fileVirtual *)this;
		return impl->size();
	}

	holder<fileHandle> newFile(const string &path, const fileMode &mode)
	{
		string p;
		auto a = archiveFindTowardsRoot(path, false, p);
		if (a)
			return a->openFile(p, mode);
		else
			return realNewFile(path, mode);
	}

	namespace
	{
		class fileReal : public fileVirtual
		{
		public:
			FILE *f;

			fileReal(const string &path, const fileMode &mode) : fileVirtual(path, mode), f(nullptr)
			{
				CAGE_ASSERT_RUNTIME(mode.valid(), "invalid fileHandle mode", path, mode.read, mode.write, mode.append, mode.textual);
				realCreateDirectories(pathJoin(path, ".."));
				f = fopen(path.c_str(), mode.mode().c_str());
				if (!f)
				{
					CAGE_LOG(severityEnum::Note, "exception", string("read: ") + mode.read + ", write: " + mode.write + ", append: " + mode.append + ", text: " + mode.textual);
					CAGE_LOG(severityEnum::Note, "exception", string("path: ") + path);
					CAGE_THROW_ERROR(codeException, "fopen", errno);
				}
			}

			~fileReal()
			{
				if (f)
				{
					try
					{
						close();
					}
					catch (...)
					{
						// do nothing
					}
				}
			}

			void read(void *data, uint64 size) override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				CAGE_ASSERT_RUNTIME(mode.read);
				if (size == 0)
					return;
				if (fread(data, (size_t)size, 1, f) != 1)
					CAGE_THROW_ERROR(codeException, "fread", errno);
			}

			void write(const void *data, uint64 size) override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				CAGE_ASSERT_RUNTIME(mode.write);
				if (size == 0)
					return;
				if (fwrite(data, (size_t)size, 1, f) != 1)
					CAGE_THROW_ERROR(codeException, "fwrite", errno);
			}

			void seek(uint64 position) override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				if (fseek(f, position, 0) != 0)
					CAGE_THROW_ERROR(codeException, "fseek", errno);
			}

			void flush() override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				if (fflush(f) != 0)
					CAGE_THROW_ERROR(codeException, "fflush", errno);
			}

			void close() override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				FILE *t = f;
				f = nullptr;
				if (fclose(t) != 0)
					CAGE_THROW_ERROR(codeException, "fclose", errno);
			}

			uint64 tell() const override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				return ftell(f);
			}

			uint64 size() const override
			{
				CAGE_ASSERT_RUNTIME(f, "fileHandle closed");
				uint64 pos = ftell(f);
				fseek(f, 0, 2);
				uint64 siz = ftell(f);
				fseek(f, pos, 0);
				return siz;
			}
		};
	}

	holder<fileHandle> realNewFile(const string &path, const fileMode &mode)
	{
		return detail::systemArena().createImpl<fileHandle, fileReal>(path, mode);
	}
}
