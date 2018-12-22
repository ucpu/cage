namespace cage
{
	template<class AllocatorPolicy, class ConcurrentPolicy>
	struct memoryArenaFixed
	{
		memoryArenaFixed(uintPtr size) : size(size)
		{
			origin = detail::systemArena().allocate(size, sizeof(uintPtr));
			allocator.setOrigin(origin);
			allocator.setSize(size);
		}

		~memoryArenaFixed()
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
			detail::systemArena().deallocate(origin);
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			scopeLock<ConcurrentPolicy> g(&concurrent);
			try
			{
				void *tmp = allocator.allocate(size, alignment);
				CAGE_ASSERT_RUNTIME(numeric_cast<uintPtr>(tmp) >= numeric_cast<uintPtr>(origin), "allocator corrupted", tmp, origin, size);
				CAGE_ASSERT_RUNTIME(numeric_cast<uintPtr>(tmp) + size <= numeric_cast<uintPtr>(origin) + this->size, "allocator corrupted", tmp, origin, size, this->size);
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

		const void *getOrigin() const { return origin; }
		uintPtr getCurrentSize() const { return size; }
		uintPtr getReservedSize() const { return size; }

	private:
		AllocatorPolicy allocator;
		ConcurrentPolicy concurrent;
		void *origin;
		const uintPtr size;
	};
}
