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
	FileAbstract::FileAbstract(const string &path, const FileMode &mode) : myPath(path), mode(mode)
	{}

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

	void File::read(void *data, uintPtr size)
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->read(data, size);
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
		uintPtr s = numeric_cast<uint32>(min(origLeft, (uintPtr)string::MaxLength));
		read(buffer, s);
		const char *b = buffer;
		if (!detail::readLine(line, b, s, origLeft >= string::MaxLength))
		{
			seek(origPos);
			if (origLeft >= string::MaxLength)
				CAGE_THROW_ERROR(Exception, "line too long");
			return false;
		}
		seek(min(origPos + (b - buffer), origSize));
		return true;
	}

	MemoryBuffer File::readBuffer(uintPtr size)
	{
		MemoryBuffer r(size);
		read(r.data(), r.size());
		return r;
	}

	void File::write(const void *data, uintPtr size)
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->write(data, size);
	}

	void File::writeLine(const string &data)
	{
		string d = data + "\n";
		write(d.c_str(), d.length());
	}

	void File::writeBuffer(const MemoryBuffer &buffer)
	{
		write(buffer.data(), buffer.size());
	}

	void File::seek(uintPtr position)
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->seek(position);
	}

	void File::flush()
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->flush();
	}

	void File::close()
	{
		FileAbstract *impl = (FileAbstract *)this;
		impl->close();
	}

	uintPtr File::tell() const
	{
		FileAbstract *impl = (FileAbstract *)this;
		return impl->tell();
	}

	uintPtr File::size() const
	{
		FileAbstract *impl = (FileAbstract *)this;
		return impl->size();
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

	namespace
	{
		class FileReal : public FileAbstract
		{
		public:
			FILE *f;

			FileReal(const string &path, const FileMode &mode) : FileAbstract(path, mode), f(nullptr)
			{
				CAGE_ASSERT(mode.valid(), "invalid file mode", path, mode.read, mode.write, mode.append, mode.textual);
				realCreateDirectories(pathJoin(path, ".."));
				f = fopen(path.c_str(), mode.mode().c_str());
				if (!f)
				{
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "read: " + mode.read + ", write: " + mode.write + ", append: " + mode.append + ", text: " + mode.textual);
					CAGE_LOG(SeverityEnum::Note, "exception", stringizer() + "path: " + path);
					CAGE_THROW_ERROR(SystemError, "fopen", errno);
				}
			}

			~FileReal()
			{
				if (f)
				{
					try
					{
						close();
					}
					catch (const cage::Exception &)
					{
						// do nothing
					}
				}
			}

			void read(void *data, uintPtr size) override
			{
				CAGE_ASSERT(f, "file closed");
				CAGE_ASSERT(mode.read);
				if (size == 0)
					return;
				if (fread(data, size, 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fread", errno);
			}

			void write(const void *data, uintPtr size) override
			{
				CAGE_ASSERT(f, "file closed");
				CAGE_ASSERT(mode.write);
				if (size == 0)
					return;
				if (fwrite(data, size, 1, f) != 1)
					CAGE_THROW_ERROR(SystemError, "fwrite", errno);
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(f, "file closed");
				if (fseek(f, position, 0) != 0)
					CAGE_THROW_ERROR(SystemError, "fseek", errno);
			}

			void flush() override
			{
				CAGE_ASSERT(f, "file closed");
				if (fflush(f) != 0)
					CAGE_THROW_ERROR(SystemError, "fflush", errno);
			}

			void close() override
			{
				CAGE_ASSERT(f, "file closed");
				FILE *t = f;
				f = nullptr;
				if (fclose(t) != 0)
					CAGE_THROW_ERROR(SystemError, "fclose", errno);
			}

			uintPtr tell() const override
			{
				CAGE_ASSERT(f, "file closed");
				return numeric_cast<uintPtr>(ftell(f));
			}

			uintPtr size() const override
			{
				CAGE_ASSERT(f, "file closed");
				auto pos = ftell(f);
				fseek(f, 0, 2);
				auto siz = ftell(f);
				fseek(f, pos, 0);
				return numeric_cast<uintPtr>(siz);
			}
		};
	}

	Holder<File> realNewFile(const string &path, const FileMode &mode)
	{
		return detail::systemArena().createImpl<File, FileReal>(path, mode);
	}
}
