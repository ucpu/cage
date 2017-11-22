#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/memoryBuffer.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#define fseek _fseeki64
#define ftell _ftelli64
#else
#define _FILE_OFFSET_BITS 64
#endif

#include <cstdio>
#include <cerrno>

namespace cage
{
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

	namespace
	{
		struct fileImpl : public fileClass
		{
			string name;
			FILE *f;
			fileMode mode;

			fileImpl(const string &name, const fileMode &mode) : name(name), f(nullptr), mode(mode)
			{
				CAGE_ASSERT_RUNTIME(mode.valid(), "invalid file mode", name, mode.read, mode.write, mode.append, mode.textual);
				pathCreateDirectories(pathExtractPath(name));
				f = fopen(name.c_str(), mode.mode().c_str());
				if (!f)
				{
					CAGE_LOG(severityEnum::Note, "exception", string("read: ") + mode.read + ", write: " + mode.write + ", append: " + mode.append + ", text: " + mode.textual);
					CAGE_LOG(severityEnum::Note, "exception", string("name: ") + name);
					CAGE_THROW_ERROR(codeException, "fopen", errno);
				}
			}

			~fileImpl()
			{
				if (f)
				{
					if (fclose(f) != 0)
					{
						detail::terminate();
					}
				}
			}
		};
	}

	void fileClass::read(void *data, uint64 size)
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		if (size == 0)
			return;
		if (fread(data, (size_t)size, 1, impl->f) != 1)
			CAGE_THROW_ERROR(codeException, "fread", errno);
	}

	bool fileClass::readLine(string &line)
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		if (feof(impl->f))
			return false;
		line = "";
		while (true)
		{
			int c = fgetc(impl->f);
			if (c == EOF)
				return true;
			if (c == '\n')
			{
#ifdef CAGE_SYSTEM_WINDOWS
				if (!line.empty() && line[line.length() - 1] == '\r')
					line = line.subString(0, line.length() - 1);
#endif
				return true;
			}
			char cc = c;
			line += string(&cc, 1);
		}
	}

	memoryBuffer fileClass::readBuffer(uintPtr size)
	{
		memoryBuffer r(size);
		read(r.data(), r.size());
		return r;
	}

	void fileClass::write(const void *data, uint64 size)
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		if (size == 0)
			return;
		if (fwrite(data, (size_t)size, 1, impl->f) != 1)
			CAGE_THROW_ERROR(codeException, "fwrite", errno);
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
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		if (fseek(impl->f, position, 0) != 0)
			CAGE_THROW_ERROR(codeException, "fseek", errno);
	}

	void fileClass::reopen(const fileMode &mode)
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(mode.valid(), "invalid file mode", impl->name, mode.read, mode.write, mode.append, mode.textual);
		impl->f = freopen(nullptr, mode.mode().c_str(), impl->f);
		if (!impl->f)
			CAGE_THROW_ERROR(codeException, "freopen", errno);
		impl->mode = mode;
	}

	void fileClass::flush()
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		if (fflush(impl->f) != 0)
			CAGE_THROW_ERROR(codeException, "fflush", errno);
	}

	void fileClass::close()
	{
		fileImpl *impl = (fileImpl *)this;
		if (impl->f)
		{
			FILE *f = impl->f;
			impl->f = nullptr;
			if (fclose(f) != 0)
				CAGE_THROW_ERROR(codeException, "fclose", errno);
		}
	}

	uint64 fileClass::tell() const
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		return ftell(impl->f);
	}

	uint64 fileClass::size() const
	{
		fileImpl *impl = (fileImpl *)this;
		CAGE_ASSERT_RUNTIME(impl->f, "file closed");
		uint64 pos = ftell(impl->f);
		fseek(impl->f, 0, 2);
		uint64 siz = ftell(impl->f);
		fseek(impl->f, pos, 0);
		return siz;
	}

	string fileClass::name() const
	{
		fileImpl *impl = (fileImpl *)this;
		return pathExtractFilename(impl->name);
	}

	string fileClass::path() const
	{
		fileImpl *impl = (fileImpl *)this;
		return pathExtractPath(impl->name);
	}

	fileMode fileClass::mode() const
	{
		fileImpl *impl = (fileImpl *)this;
		return impl->mode;
	}

	holder<fileClass> newFile(const string &name, const fileMode &mode)
	{
		return detail::systemArena().createImpl<fileClass, fileImpl>(name, mode);
	}
}
