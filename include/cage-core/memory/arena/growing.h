namespace cage
{
	template<class AllocatorPolicy, class ConcurrentPolicy>
	struct memoryArenaGrowing
	{
		memoryArenaGrowing(uintPtr addressSize, uintPtr initialSize = 1024 * 1024) : origin(nullptr), pageSize(detail::memoryPageSize()), addressSize(detail::roundUpTo(addressSize, pageSize)), currentSize(0)
		{
			memory = newVirtualMemory();
			origin = memory->reserve(this->addressSize / pageSize);
			currentSize = detail::roundUpTo(initialSize <= addressSize ? initialSize : addressSize, pageSize);
			if (currentSize)
				memory->increase(currentSize / pageSize);
			allocator.setOrigin(origin);
			allocator.setSize(currentSize);
		}

		~memoryArenaGrowing()
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
			try
			{
				return alloc(size, alignment);
			}
			catch (const outOfMemoryException &e)
			{
				// allocate one more third plus some constant
				uintPtr adding = detail::roundUpTo((memory->pages() + 1) * pageSize / 3 + e.memory, pageSize);
				memory->increase(adding / pageSize);
				currentSize += adding;
				allocator.setSize(currentSize);
			}
			try
			{
				return alloc(size, alignment);
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

		const void *getOrigin() const { return origin; }
		uintPtr getCurrentSize() const { return currentSize; }
		uintPtr getReservedSize() const { return addressSize; }

	private:
		holder<virtualMemoryClass> memory;
		AllocatorPolicy allocator;
		ConcurrentPolicy concurrent;
		const uintPtr pageSize;
		void *origin;
		const uintPtr addressSize;
		uintPtr currentSize;

		void *alloc(uintPtr size, uintPtr alignment)
		{
			void *tmp = allocator.allocate(size, alignment);
			CAGE_ASSERT_RUNTIME(tmp >= origin, "allocator corrupted", tmp, origin, size);
			CAGE_ASSERT_RUNTIME((char*)tmp + size <= (char*)origin + currentSize, "allocator corrupted", tmp, origin, size, currentSize);
			return tmp;
		}
	};
}
