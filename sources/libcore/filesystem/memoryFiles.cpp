#include <cage-core/memoryBuffer.h>

#include "files.h"

namespace cage
{
	namespace
	{
		class FileBuffer : public FileAbstract
		{
		public:
			MemoryBuffer persistent;
			MemoryBuffer &buf;
			uintPtr pos = 0;

			FileBuffer(MemoryBuffer *buffer, const FileMode &mode) : FileAbstract(stringizer() + "buffer:/" + uintPtr(buffer), mode), buf(*buffer)
			{
				CAGE_ASSERT(mode.valid());
				if (mode.append)
					pos = buf.size();
			}

			FileBuffer(MemoryBuffer &&buffer, const FileMode &mode) : FileAbstract(stringizer() + "buffer:/" + uintPtr(&persistent), mode), persistent(std::move(buffer)), buf(persistent)
			{
				CAGE_ASSERT(mode.valid());
				if (mode.append)
					pos = buf.size();
			}

			void read(PointerRange<char> buffer) override
			{
				if (!mode.read)
					CAGE_THROW_CRITICAL(NotImplemented, "reading from write-only memory file");
				char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf.size())
					CAGE_THROW_ERROR(Exception, "reading beyond buffer");
				detail::memcpy(data, buf.data() + pos, size);
				pos += size;
			}

			void write(PointerRange<const char> buffer) override
			{
				if (!mode.write)
					CAGE_THROW_CRITICAL(NotImplemented, "writing to read-only memory file");
				const char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf.size())
					buf.resizeSmart(pos + size);
				detail::memcpy(buf.data() + pos, data, size);
				pos += size;
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(position <= buf.size());
				pos = position;
			}

			void close() override
			{
				// nothing
			}

			uintPtr tell() const override
			{
				return pos;
			}

			uintPtr size() const override
			{
				return buf.size();
			}
		};

		class FileRange : public FileAbstract
		{
		public:
			const PointerRange<char> buf;
			uintPtr pos = 0;

			FileRange(PointerRange<char> buffer, const FileMode &mode) : FileAbstract(stringizer() + "memory:/" + uintPtr(buffer.data()), mode), buf(buffer)
			{
				CAGE_ASSERT(mode.valid());
				if (mode.append)
					pos = buf.size();
			}

			void read(PointerRange<char> buffer) override
			{
				if (!mode.read)
					CAGE_THROW_CRITICAL(NotImplemented, "reading from write-only memory file");
				char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf.size())
					CAGE_THROW_ERROR(Exception, "reading beyond buffer");
				detail::memcpy(data, buf.data() + pos, size);
				pos += size;
			}

			void write(PointerRange<const char> buffer) override
			{
				if (!mode.write)
					CAGE_THROW_CRITICAL(NotImplemented, "writing to read-only memory file");
				const char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf.size())
					CAGE_THROW_ERROR(Exception, "writing beyond buffer");
				detail::memcpy(buf.data() + pos, data, size);
				pos += size;
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(position <= buf.size());
				pos = position;
			}

			void close() override
			{
				// nothing
			}

			uintPtr tell() const override
			{
				return pos;
			}

			uintPtr size() const override
			{
				return buf.size();
			}
		};
	}

	Holder<File> newFileBuffer(MemoryBuffer *buffer, const FileMode &mode)
	{
		return detail::systemArena().createImpl<File, FileBuffer>(buffer, mode);
	}

	Holder<File> newFileBuffer(MemoryBuffer &&buffer, const FileMode &mode)
	{
		return detail::systemArena().createImpl<File, FileBuffer>(templates::move(buffer), mode);
	}

	Holder<File> newFileBuffer(PointerRange<char> buffer, const FileMode &mode)
	{
		return detail::systemArena().createImpl<File, FileRange>(buffer, mode);
	}

	Holder<File> newFileBuffer(PointerRange<const char> buffer)
	{
		return detail::systemArena().createImpl<File, FileRange>(PointerRange<char>((char*)buffer.begin(), (char*)buffer.end()), FileMode(true, false));
	}

	Holder<File> newFileBuffer()
	{
		return detail::systemArena().createImpl<File, FileBuffer>(MemoryBuffer(), FileMode(true, true));
	}
}
