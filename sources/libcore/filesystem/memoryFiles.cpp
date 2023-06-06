#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	namespace
	{
		class FileBuffer : public File
		{
		public:
			const FileMode myMode;
			Holder<MemoryBuffer> buf;
			uintPtr pos = 0;

			FileBuffer(Holder<MemoryBuffer> buffer, const FileMode &mode) : myMode(mode), buf(std::move(buffer))
			{
				CAGE_ASSERT(mode.valid());
				if (mode.append)
					pos = buf->size();
			}

			void read(PointerRange<char> buffer) override
			{
				if (!myMode.read)
					CAGE_THROW_CRITICAL(NotImplemented, "reading from write-only memory file");
				char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf->size())
					CAGE_THROW_ERROR(Exception, "reading beyond buffer");
				if (size)
					detail::memcpy(data, buf->data() + pos, size);
				pos += size;
			}

			void write(PointerRange<const char> buffer) override
			{
				if (!myMode.write)
					CAGE_THROW_CRITICAL(NotImplemented, "writing to read-only memory file");
				const char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf->size())
					buf->resizeSmart(pos + size);
				if (size)
					detail::memcpy(buf->data() + pos, data, size);
				pos += size;
			}

			void seek(uintPtr position) override
			{
				CAGE_ASSERT(position <= buf->size());
				pos = position;
			}

			void close() override
			{
				// nothing
			}

			uintPtr tell() override { return pos; }

			uintPtr size() override { return buf->size(); }

			FileMode mode() const override { return myMode; }
		};

		class FileRange : public File
		{
		public:
			const FileMode myMode;
			Holder<PointerRange<char>> buf;
			uintPtr pos = 0;

			FileRange(Holder<PointerRange<char>> buffer, const FileMode &mode) : buf(std::move(buffer)), myMode(mode)
			{
				CAGE_ASSERT(mode.valid());
				if (mode.append)
					pos = buf.size();
			}

			void read(PointerRange<char> buffer) override
			{
				if (!myMode.read)
					CAGE_THROW_CRITICAL(NotImplemented, "reading from write-only memory file");
				char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf.size())
					CAGE_THROW_ERROR(Exception, "reading beyond buffer");
				if (size)
					detail::memcpy(data, buf.data() + pos, size);
				pos += size;
			}

			void write(PointerRange<const char> buffer) override
			{
				if (!myMode.write)
					CAGE_THROW_CRITICAL(NotImplemented, "writing to read-only memory file");
				const char *data = buffer.data();
				const uintPtr size = buffer.size();
				if (pos + size > buf.size())
					CAGE_THROW_ERROR(Exception, "writing beyond buffer");
				if (size)
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

			uintPtr tell() override { return pos; }

			uintPtr size() override { return buf.size(); }

			FileMode mode() const override { return myMode; }
		};
	}

	Holder<File> newFileBuffer(Holder<PointerRange<const char>> buffer)
	{
		PointerRange<char> *r = (PointerRange<char> *)+buffer;
		Holder<PointerRange<char>> tmp = Holder<PointerRange<char>>(r, std::move(buffer));
		return newFileBuffer(std::move(tmp), FileMode(true, false));
	}

	Holder<File> newFileBuffer(Holder<PointerRange<char>> buffer, const FileMode &mode)
	{
		return systemMemory().createImpl<File, FileRange>(std::move(buffer), mode);
	}

	Holder<File> newFileBuffer(Holder<const MemoryBuffer> buffer)
	{
		MemoryBuffer *b = (MemoryBuffer *)+buffer;
		return newFileBuffer(Holder<MemoryBuffer>(b, std::move(buffer)), FileMode(true, false));
	}

	Holder<File> newFileBuffer(Holder<MemoryBuffer> buffer, const FileMode &mode)
	{
		return systemMemory().createImpl<File, FileBuffer>(std::move(buffer), mode);
	}

	Holder<File> newFileBuffer(MemoryBuffer &&buffer, const FileMode &mode)
	{
		Holder<MemoryBuffer> tmp = systemMemory().createHolder<MemoryBuffer>(std::move(buffer));
		return newFileBuffer(std::move(tmp), mode);
	}

	Holder<File> newFileBuffer()
	{
		return newFileBuffer(MemoryBuffer());
	}
}
