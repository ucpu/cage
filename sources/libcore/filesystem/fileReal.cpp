#include "filesystem.h"

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
	namespace
	{
		struct fileReal : public fileVirtual
		{
			FILE *f;

			fileReal(const string &name, const fileMode &mode) : fileVirtual(name, mode), f(nullptr)
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

			~fileReal()
			{
				if (f)
				{
					if (fclose(f) != 0)
					{
						detail::terminate();
					}
				}
			}

			void read(void *data, uint64 size) override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				if (size == 0)
					return;
				if (fread(data, (size_t)size, 1, f) != 1)
					CAGE_THROW_ERROR(codeException, "fread", errno);
			}

			void write(const void *data, uint64 size) override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
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

			void reopen(const fileMode &mode) override
			{
				CAGE_ASSERT_RUNTIME(mode.valid(), "invalid file mode", name, mode.read, mode.write, mode.append, mode.textual);
				f = freopen(nullptr, mode.mode().c_str(), f);
				if (!f)
					CAGE_THROW_ERROR(codeException, "freopen", errno);
				this->mode = mode;
			}

			void flush() override
			{
				CAGE_ASSERT_RUNTIME(f, "file closed");
				if (fflush(f) != 0)
					CAGE_THROW_ERROR(codeException, "fflush", errno);
			}

			void close() override
			{
				if (f)
				{
					FILE *t = f;
					f = nullptr;
					if (fclose(t) != 0)
						CAGE_THROW_ERROR(codeException, "fclose", errno);
				}
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

	holder<fileClass> realNewFile(const string &name, const fileMode &mode)
	{
		return detail::systemArena().createImpl<fileClass, fileReal>(name, mode);
	}
}
