#include "files.h"

namespace cage
{
	// provides a limited section of a file as another file
	struct ProxyFile final : public FileAbstract
	{
		FileAbstract *f = nullptr;
		const uint64 start = 0;
		const uint64 capacity = 0;
		uint64 off = 0;

		ProxyFile(FileAbstract *f, uint64 start, uint64 capacity) : FileAbstract(f->myPath, FileMode(true, false)), f(f), start(start), capacity(capacity) {}

		~ProxyFile() {}

		void readAt(PointerRange<char> buffer, uint64 at) override
		{
			ScopeLock lock(fsMutex());
			CAGE_ASSERT(f);
			CAGE_ASSERT(buffer.size() <= capacity - at);
			f->readAt(buffer, start + at);
		}

		void read(PointerRange<char> buffer) override
		{
			ScopeLock lock(fsMutex());
			CAGE_ASSERT(f);
			CAGE_ASSERT(buffer.size() <= capacity - off);
			f->seek(start + off);
			f->read(buffer);
			off += buffer.size();
		}

		void seek(uint64 position) override
		{
			ScopeLock lock(fsMutex());
			CAGE_ASSERT(f);
			CAGE_ASSERT(position <= capacity);
			off = position;
		}

		void close() override
		{
			ScopeLock lock(fsMutex());
			f = nullptr;
		}

		uint64 tell() override
		{
			ScopeLock lock(fsMutex());
			return off;
		}

		uint64 size() override
		{
			ScopeLock lock(fsMutex());
			return capacity;
		}
	};

	Holder<File> newProxyFile(File *f, uint64 start, uint64 size)
	{
		return systemMemory().createImpl<File, ProxyFile>(class_cast<FileAbstract *>(f), start, size);
	}
}
