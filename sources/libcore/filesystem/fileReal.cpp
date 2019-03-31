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

	holder<fileClass> realNewFile(const string &path, const fileMode &mode)
	{
		return detail::systemArena().createImpl<fileClass, fileReal>(path, mode);
	}
}
