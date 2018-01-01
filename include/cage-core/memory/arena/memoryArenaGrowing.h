namespace cage
{
	template<class AllocatorPolicy, class ConcurrentPolicy>
	struct memoryArenaGrowing
	{
		memoryArenaGrowing(uintPtr addressSize, uintPtr initialSize = 1) : addressSize(detail::roundUpToMemoryPage(addressSize)), currentSize(0)
		{
			memory = newVirtualMemory();
			origin = memory->reserve(this->addressSize / detail::memoryPageSize());
			currentSize = detail::roundUpToMemoryPage(initialSize);
			if (currentSize)
				memory->increase(currentSize / detail::memoryPageSize());
			allocator.setOrigin(origin);
			allocator.setSize(currentSize);
		}

		~memoryArenaGrowing()
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
		}

		void *allocate(uintPtr size)
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
			try
			{
				void *tmp = allocator.allocate(size);
				CAGE_ASSERT_RUNTIME(tmp >= origin, "allocator corrupted", tmp, origin, size);
				CAGE_ASSERT_RUNTIME((char*)tmp + size <= (char*)origin + currentSize, "allocator corrupted", tmp, origin, size, currentSize);
				return tmp;
			}
			catch (const outOfMemoryException &e)
			{
				uintPtr adding = detail::roundUpToMemoryPage(e.memory) + detail::memoryPageSize();
				memory->increase(adding / detail::memoryPageSize());
				currentSize += adding;
				allocator.setSize(currentSize);
			}
			try
			{
				void *tmp = allocator.allocate(size);
				CAGE_ASSERT_RUNTIME(tmp >= origin, "allocator corrupted", tmp, origin, size);
				CAGE_ASSERT_RUNTIME((char*)tmp + size <= (char*)origin + currentSize, "allocator corrupted", tmp, origin, size, currentSize);
				return tmp;
			}
			catch (outOfMemoryException &e)
			{
				e.severity = severityEnum::Error;
				e.log();
				throw;
			}
		}

		void deallocate(void *ptr)
		{
			if (ptr == nullptr)
				return;
			scopeLock<ConcurrentPolicy> g(&concurrent);
			allocator.deallocate(ptr);
		}

		void flush()
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
			allocator.flush();
		}

	private:
		holder<virtualMemoryClass> memory;
		AllocatorPolicy allocator;
		ConcurrentPolicy concurrent;
		void *origin;
		const uintPtr addressSize;
		uintPtr currentSize;
	};
}
