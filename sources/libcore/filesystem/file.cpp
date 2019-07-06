#include "filesystem.h"
#include <cage-core/memoryBuffer.h>
#include <cage-core/math.h> // min
#include <cage-core/lineReader.h>

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

		const uint64 origPos = tell();
		const uint64 origSize = size();
		const uint64 origLeft = origSize - origPos;
		if (origLeft == 0)
			return false;

		char buffer[string::MaxLength + 1];
		uintPtr s = numeric_cast<uint32>(min(origLeft, (uint64)string::MaxLength));
		read(buffer, s);
		const char *b = buffer;
		if (!detail::readLine(line, b, s, origLeft >= string::MaxLength))
		{
			seek(origPos);
			if (origLeft >= string::MaxLength)
				CAGE_THROW_ERROR(exception, "line too long");
			return false;
		}
		seek(min(origPos + (b - buffer), origSize));
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
				CAGE_ASSERT_RUNTIME(mode.valid(), "invalid file mode", path, mode.read, mode.write, mode.append, mode.textual);
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
				CAGE_ASSERT_RUNTIME(f, "file closed");
				CAGE_ASSERT_RUNTIME(mode.read);
				if (size == 0)
					return;
				if (fread(data, (size_t)size, 1, f) != 1)
					CAGE_THROW_ERROR(codeException, "fread", errno);
			}

			void write(const void *data, uint64 size) override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				CAGE_ASSERT_RUNTIME(mode.write);
				if (size == 0)
					return;
				if (fwrite(data, (size_t)size, 1, f) != 1)
					CAGE_THROW_ERROR(codeException, "fwrite", errno);
			}

			void seek(uint64 position) override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				if (fseek(f, position, 0) != 0)
					CAGE_THROW_ERROR(codeException, "fseek", errno);
			}

			void flush() override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				if (fflush(f) != 0)
					CAGE_THROW_ERROR(codeException, "fflush", errno);
			}

			void close() override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				FILE *t = f;
				f = nullptr;
				if (fclose(t) != 0)
					CAGE_THROW_ERROR(codeException, "fclose", errno);
			}

			uint64 tell() const override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				return ftell(f);
			}

			uint64 size() const override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
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
