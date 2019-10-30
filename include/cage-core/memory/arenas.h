namespace cage
{
	template<class AllocatorPolicy, class ConcurrentPolicy>
	struct memoryArenaFixed
	{
		explicit memoryArenaFixed(uintPtr size) : size(size)
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
				CAGE_ASSERT(uintPtr(tmp) >= uintPtr(origin), "allocator corrupted", tmp, origin, size);
				CAGE_ASSERT(uintPtr(tmp) + size <= uintPtr(origin) + this->size, "allocator corrupted", tmp, origin, size, this->size);
				return tmp;
			}
			catch (outOfMemory &e)
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

	template<class AllocatorPolicy, class ConcurrentPolicy>
	struct memoryArenaGrowing
	{
		explicit memoryArenaGrowing(uintPtr addressSize, uintPtr initialSize = 1024 * 1024) : origin(nullptr), pageSize(detail::memoryPageSize()), addressSize(detail::roundUpTo(addressSize, pageSize)), currentSize(0)
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
			catch (const outOfMemory &e)
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
			catch (outOfMemory &e)
			{
				e.severity = severityEnum::Error;
				e.makeLog();
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
			CAGE_ASSERT(tmp >= origin, "allocator corrupted", tmp, origin, size);
			CAGE_ASSERT((char*)tmp + size <= (char*)origin + currentSize, "allocator corrupted", tmp, origin, size, currentSize);
			return tmp;
		}
	};

	template<class T>
	struct memoryArenaStd
	{
		typedef T value_type;

		memoryArenaStd(const memoryArena &other) : a(other) {}
		memoryArenaStd(const memoryArenaStd &other) : a(other.a) {}

		template<class U>
		explicit memoryArenaStd(const memoryArenaStd<U> &other) : a(other.a) {}

		T *allocate(uintPtr cnt)
		{
			return (T*)a.allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(T *ptr, uintPtr) noexcept
		{
			a.deallocate(ptr);
		}

		bool operator == (const memoryArenaStd &other) const noexcept
		{
			return a == other.a;
		}

		bool operator != (const memoryArenaStd &other) const noexcept
		{
			return a != other.a;
		}

	private:
		memoryArena a;
		template<class U>
		friend struct memoryArenaStd;
	};
}

