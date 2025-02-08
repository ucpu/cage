#include "files.h"

namespace cage
{
	// provides a limited section of a file as another file
	struct ProxyFile final : public FileAbstract
	{
		FileAbstract *f = nullptr;
		const uintPtr start = 0;
		const uintPtr capacity = 0;
		uintPtr off = 0;

		ProxyFile(FileAbstract *f, uintPtr start, uintPtr capacity) : FileAbstract(f->myPath, FileMode(true, false)), f(f), start(start), capacity(capacity) {}

		~ProxyFile() {}

		void readAt(PointerRange<char> buffer, uintPtr at) override
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

		void seek(uintPtr position) override
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

		uintPtr tell() override
		{
			ScopeLock lock(fsMutex());
			return off;
		}

		uintPtr size() override
		{
			ScopeLock lock(fsMutex());
			return capacity;
		}
	};

	Holder<File> newProxyFile(File *f, uintPtr start, uintPtr size)
	{
		return systemMemory().createImpl<File, ProxyFile>(class_cast<FileAbstract *>(f), start, size);
	}
}
