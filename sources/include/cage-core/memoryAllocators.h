#ifndef guard_memoryAllocators_h_dsg4hsd564g6s5e1gse5a64fg
#define guard_memoryAllocators_h_dsg4hsd564g6s5e1gse5a64fg

#include "memoryUtils.h"

namespace cage
{
	namespace templates
	{
		template<class T>
		struct PoolAllocatorAtomSize
		{
			static constexpr uintPtr result = sizeof(T) + alignof(T);
		};

		template<class T>
		struct AllocatorSizeList
		{
			void *a;
			void *b;
			T t;
		};

		template<class T>
		struct AllocatorSizeSet
		{
#ifdef CAGE_SYSTEM_WINDOWS
			void *a;
			void *b;
			void *c;
			char d;
			char e;
			T t;
#else
			char d;
			void *a;
			void *b;
			void *c;
			T t;
#endif
		};

		template<class K, class V>
		struct AllocatorSizeMap
		{
			struct pair
			{
				K k;
				V v;
			};
			AllocatorSizeSet<pair> s;
		};
	}

	struct CAGE_CORE_API MemoryBoundsPolicyNone
	{
		static constexpr uintPtr SizeFront = 0;
		static constexpr uintPtr SizeBack = 0;

		void setFront(void *ptr) {}
		void setBack(void *ptr) {}
		void checkFront(void *ptr) {}
		void checkBack(void *ptr) {}
	};

	struct CAGE_CORE_API MemoryBoundsPolicySimple
	{
		static constexpr uintPtr SizeFront = 4;
		static constexpr uintPtr SizeBack = 4;

		void setFront(void *ptr)
		{
			*(uint32*)ptr = PatternFront;
		}

		void setBack(void *ptr)
		{
			*(uint32*)ptr = PatternBack;
		}

		void checkFront(void *ptr)
		{
			if (*(uint32*)ptr != PatternFront)
				CAGE_THROW_CRITICAL(Exception, "memory corruption detected");
		}

		void checkBack(void *ptr)
		{
			if (*(uint32*)ptr != PatternBack)
				CAGE_THROW_CRITICAL(Exception, "memory corruption detected");
		}

	private:
		static constexpr uint32 PatternFront = 0xCDCDCDCD;
		static constexpr uint32 PatternBack = 0xDCDCDCDC;
	};

	struct CAGE_CORE_API MemoryTagPolicyNone
	{
		void set(void *ptr, uintPtr size) {}
		void check(void *ptr, uintPtr size) {}
	};

	struct CAGE_CORE_API MemoryTagPolicySimple
	{
		void set(void *ptr, uintPtr size)
		{
			uintPtr s = size / 2;
			for (uintPtr i = 0; i < s; i++)
				*((uint16*)ptr + i) = 0xBEAF;
		}

		void check(void *ptr, uintPtr size)
		{
			uintPtr s = size / 2;
			for (uintPtr i = 0; i < s; i++)
				*((uint16*)ptr + i) = 0xDEAD;
		}
	};

	struct CAGE_CORE_API MemoryTrackPolicyNone
	{
		void set(void *ptr, uintPtr size) {}
		void check(void *ptr) {}
		void flush() {}
	};

	struct CAGE_CORE_API MemoryTrackPolicySimple
	{
		MemoryTrackPolicySimple() {}
		~MemoryTrackPolicySimple();

		void set(void *ptr, const uintPtr size)
		{
			count++;
		}

		void check(void *ptr)
		{
			if (count == 0)
				CAGE_THROW_CRITICAL(Exception, "double deallocation detected");
			count--;
		}

		void flush()
		{
			count = 0;
		}

	private:
		uint32 count = 0;
	};

	struct CAGE_CORE_API MemoryTrackPolicyAdvanced
	{
	public:
		MemoryTrackPolicyAdvanced();
		~MemoryTrackPolicyAdvanced();
		void set(void *ptr, uintPtr size);
		void check(void *ptr);
		void flush();

	private:
		void *data;
	};

	struct CAGE_CORE_API MemoryConcurrentPolicyNone
	{
		void lock() {}
		void unlock() {}
	};

	struct CAGE_CORE_API MemoryConcurrentPolicyMutex
	{
		MemoryConcurrentPolicyMutex();
		void lock();
		void unlock();

	private:
		Holder<Mutex> mutex;
	};

#ifdef CAGE_DEBUG
	using MemoryBoundsPolicyDefault = MemoryBoundsPolicySimple;
	using MemoryTagPolicyDefault = MemoryTagPolicySimple;
	using MemoryTrackPolicyDefault = MemoryTrackPolicySimple;
#else
	using MemoryBoundsPolicyDefault = MemoryBoundsPolicyNone;
	using MemoryTagPolicyDefault = MemoryTagPolicyNone;
	using MemoryTrackPolicyDefault = MemoryTrackPolicyNone;
#endif

	template<class BoundsPolicy = MemoryBoundsPolicyDefault, class TaggingPolicy = MemoryTagPolicyDefault, class TrackingPolicy = MemoryTrackPolicyDefault>
	struct MemoryAllocatorPolicyLinear
	{
		MemoryAllocatorPolicyLinear() : origin(nullptr), current(nullptr), totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT(!origin);
			current = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT(size >= numeric_cast<uintPtr>((char*)current - (char*)origin));
			totalSize = size;
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			uintPtr alig = detail::addToAlign((uintPtr)current + BoundsPolicy::SizeFront, alignment);
			uintPtr total = alig + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if ((char*)current + total > (char*)origin + totalSize)
				CAGE_THROW_SILENT(OutOfMemory, "out of memory", total);

			void *result = (char*)current + alig + BoundsPolicy::SizeFront;
			CAGE_ASSERT((uintPtr)result % alignment == 0);

			bound.setFront((char*)result - BoundsPolicy::SizeFront);
			tag.set(result, size);
			bound.setBack((char*)result + size);
			track.set(result, size);

			current = (char*)current + total;
			return result;
		}

		void deallocate(void *ptr)
		{
			CAGE_THROW_CRITICAL(Exception, "not allowed, linear allocator must be flushed");
		}

		void flush()
		{
			track.flush();
			tag.check(origin, (char*)current - (char*)origin);
			current = origin;
		}

	private:
		BoundsPolicy bound;
		TaggingPolicy tag;
		TrackingPolicy track;
		void *origin, *current;
		uintPtr totalSize;
	};

	/*
	template<uint8 N, class BoundsPolicy = MemoryBoundsPolicyDefault, class TaggingPolicy = MemoryTagPolicyDefault, class TrackingPolicy = MemoryTrackPolicyDefault>
	struct MemoryAllocatorPolicyNFrame
	{
		MemoryAllocatorPolicyNFrame() : origin(nullptr), totalSize(0), current(0) {}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT(!origin);
			origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT(size > 0 && totalSize == 0);
			totalSize = size;
			for (uint8 i = 0; i < N; i++)
			{
				allocs[i].setOrigin((char*)origin + i * size / N);
				allocs[i].setSize(size / N);
			}
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			return allocs[current].allocate(size, alignment);
		}

		void deallocate(void *ptr)
		{
			CAGE_THROW_CRITICAL(Exception, "not allowed, nFrame allocator must be flushed");
		}

		void flush()
		{
			current = (current + 1) % N;
			allocs[current].flush();
		}

	private:
		MemoryAllocatorPolicyLinear<BoundsPolicy, TaggingPolicy, TrackingPolicy> allocs[N];
		void *origin;
		uintPtr totalSize;
		uint8 current;
	};
	*/

	template<uintPtr AtomSize, class BoundsPolicy = MemoryBoundsPolicyDefault, class TaggingPolicy = MemoryTagPolicyDefault, class TrackingPolicy = MemoryTrackPolicyDefault>
	struct MemoryAllocatorPolicyPool
	{
		static_assert(AtomSize > 0, "objects must be at least one byte");

		static constexpr uintPtr objectSizeInit()
		{
			uintPtr res = AtomSize + BoundsPolicy::SizeFront + BoundsPolicy::SizeBack;
			if (res < sizeof(uintPtr))
				res = sizeof(uintPtr);
			return res;
		}

		MemoryAllocatorPolicyPool() : origin(nullptr), current(nullptr), totalSize(0), objectSize(objectSizeInit())
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT(!origin);
			CAGE_ASSERT(newOrigin > (char*)objectSize);
			uintPtr addition = detail::addToAlign((uintPtr)newOrigin, objectSize);
			if (addition < BoundsPolicy::SizeFront)
				addition += objectSize;
			current = origin = (char*)newOrigin + addition;
		}

		void setSize(uintPtr size)
		{
			if (size == 0 && totalSize == 0)
				return;
			CAGE_ASSERT(size >= objectSize);
			uintPtr s = size - objectSize - detail::subtractToAlign((uintPtr)origin + size, objectSize);
			if (s == totalSize)
				return;
			CAGE_ASSERT(s > totalSize); // can only grow
			void *tmp = (char*)origin + totalSize;
			totalSize = s;
			while (tmp < (char*)origin + totalSize)
			{
				*(void**)tmp = (char*)tmp + objectSize;
				tmp = (char*)tmp + objectSize;
			}
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			CAGE_ASSERT(current >= origin && current <= (char*)origin + totalSize);

			if (current >= (char*)origin + totalSize)
				CAGE_THROW_SILENT(OutOfMemory, "out of memory", objectSize);

			void *next = *(void**)current;
			uintPtr alig = detail::addToAlign((uintPtr)current, alignment);
			CAGE_ASSERT(size + alig <= AtomSize);
			void *result = (char*)current + alig;

			bound.setFront((char*)current - BoundsPolicy::SizeFront);
			tag.set(current, AtomSize);
			bound.setBack((char*)current + AtomSize);
			track.set(result, size);

			current = next;
			return result;
		}

		void deallocate(void *ptr)
		{
			void *ptrOrig = ptr;
			CAGE_ASSERT(ptr >= origin && ptr < (char*)origin + totalSize);
			ptr = (char*)ptr - detail::subtractToAlign((uintPtr)ptr, objectSize);
			CAGE_ASSERT((uintPtr)ptr % objectSize == 0);

			track.check(ptrOrig);
			bound.checkFront((char*)ptr - BoundsPolicy::SizeFront);
			tag.check(ptr, AtomSize);
			bound.checkBack((char*)ptr + AtomSize);

			*(void**)ptr = current;
			current = ptr;
		}

		void flush()
		{
			track.flush();
			tag.check(origin, totalSize);
			current = origin;
			uintPtr tmp = totalSize;
			totalSize = 0;
			setSize(tmp);
		}

	protected:
		BoundsPolicy bound;
		TaggingPolicy tag;
		TrackingPolicy track;
		void *origin, *current;
		uintPtr totalSize;
		const uintPtr objectSize;
	};

	template<class BoundsPolicy = MemoryBoundsPolicyDefault, class TaggingPolicy = MemoryTagPolicyDefault, class TrackingPolicy = MemoryTrackPolicyDefault>
	struct MemoryAllocatorPolicyQueue
	{
		MemoryAllocatorPolicyQueue() : origin(nullptr), front(nullptr), back(nullptr), totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT(!origin);
			front = back = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT(size > 0 && totalSize == 0);
			totalSize = size;
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			uintPtr alig = detail::addToAlign((uintPtr)back + sizeof(uintPtr) + BoundsPolicy::SizeFront, alignment);
			uintPtr total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if (back < front && (char*)back + total >(char*)front)
			{
				CAGE_THROW_SILENT(OutOfMemory, "out of memory", total);
			}
			else if (back >= front)
			{
				if ((char*)back + total > (char*)origin + totalSize)
				{
					alig = detail::addToAlign((uintPtr)origin + sizeof(uintPtr) + BoundsPolicy::SizeFront, alignment);
					total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;
					if ((char*)origin + total > (char*)front)
						CAGE_THROW_SILENT(OutOfMemory, "out of memory", total);
					back = origin;
				}
			}

			void *result = (char*)back + alig + sizeof(uintPtr) + BoundsPolicy::SizeFront;
			CAGE_ASSERT((uintPtr)result % alignment == 0);

			*(uintPtr*)((char*)result - BoundsPolicy::SizeFront - sizeof(uintPtr)) = size;
			bound.setFront((char*)result - BoundsPolicy::SizeFront);
			tag.set(result, size);
			bound.setBack((char*)result + size);
			track.set(result, size);

			back = (char*)back + total;
			return result;
		}

		void deallocate(void *ptr)
		{
			CAGE_ASSERT(ptr >= origin && ptr < (char*)origin + totalSize);

			track.check(ptr);

			uintPtr size = *(uintPtr*)((char*)ptr - BoundsPolicy::SizeFront - sizeof(uintPtr));

			bound.checkFront((char*)ptr - BoundsPolicy::SizeFront);
			tag.check(ptr, size);
			bound.checkBack((char*)ptr + size);

			front = (char*)ptr + size + BoundsPolicy::SizeBack;

			if (front == back)
				front = back = origin;
		}

		void flush()
		{
			track.flush();
			tag.check(origin, totalSize);
			front = back = origin;
		}

	private:
		BoundsPolicy bound;
		TaggingPolicy tag;
		TrackingPolicy track;
		void *origin, *front, *back;
		uintPtr totalSize;
	};

	template<class BoundsPolicy = MemoryBoundsPolicyDefault, class TaggingPolicy = MemoryTagPolicyDefault, class TrackingPolicy = MemoryTrackPolicyDefault>
	struct MemoryAllocatorPolicyStack
	{
		MemoryAllocatorPolicyStack() : origin(nullptr), current(nullptr), totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT(!origin);
			current = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT(size >= numeric_cast<uintPtr>((char*)current - (char*)origin));
			totalSize = size;
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			uintPtr alig = detail::addToAlign((uintPtr)current + sizeof(uintPtr) + BoundsPolicy::SizeFront, alignment);
			uintPtr total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if ((char*)current + total > (char*)origin + totalSize)
				CAGE_THROW_SILENT(OutOfMemory, "out of memory", total);

			void *result = (char*)current + alig + sizeof(uintPtr) + BoundsPolicy::SizeFront;
			CAGE_ASSERT((uintPtr)result % alignment == 0);

			*(uintPtr*)((char*)result - BoundsPolicy::SizeFront - sizeof(uintPtr)) = alig;
			bound.setFront((char*)result - BoundsPolicy::SizeFront);
			tag.set(result, size);
			bound.setBack((char*)result + size);
			track.set(result, size);

			current = (char*)current + total;
			return result;
		}

		void deallocate(void *ptr)
		{
			CAGE_ASSERT(ptr >= origin && ptr < (char*)origin + totalSize);

			track.check(ptr);

			uintPtr size = (char*)current - (char*)ptr - BoundsPolicy::SizeBack;

			bound.checkFront((char*)ptr - BoundsPolicy::SizeFront);
			tag.check(ptr, size);
			bound.checkBack((char*)ptr + size);

			uintPtr alig = *(uintPtr*)((char*)ptr - BoundsPolicy::SizeFront - sizeof(uintPtr));
			current = (char*)ptr - alig - sizeof(uintPtr) - BoundsPolicy::SizeFront;
		}

		void flush()
		{
			track.flush();
			tag.check(origin, (char*)current - (char*)origin);
			current = origin;
		}

	private:
		BoundsPolicy bound;
		TaggingPolicy tag;
		TrackingPolicy track;
		void *origin, *current;
		uintPtr totalSize;
	};

	template<class AllocatorPolicy, class ConcurrentPolicy>
	struct MemoryArenaFixed
	{
		explicit MemoryArenaFixed(uintPtr size) : size(size)
		{
			origin = detail::systemArena().allocate(size, sizeof(uintPtr));
			allocator.setOrigin(origin);
			allocator.setSize(size);
		}

		~MemoryArenaFixed()
		{
			ScopeLock<ConcurrentPolicy> g(&concurrent);
			detail::systemArena().deallocate(origin);
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			ScopeLock<ConcurrentPolicy> g(&concurrent);
			try
			{
				void *tmp = allocator.allocate(size, alignment);
				CAGE_ASSERT(uintPtr(tmp) >= uintPtr(origin));
				CAGE_ASSERT(uintPtr(tmp) + size <= uintPtr(origin) + this->size);
				return tmp;
			}
			catch (OutOfMemory &e)
			{
				e.severity = SeverityEnum::Error;
				e.log();
				throw;
			}
		}

		void deallocate(void *ptr)
		{
			if (ptr == nullptr)
				return;
			ScopeLock<ConcurrentPolicy> g(&concurrent);
			allocator.deallocate(ptr);
		}

		void flush()
		{
			ScopeLock<ConcurrentPolicy> g(&concurrent);
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
	struct MemoryArenaGrowing
	{
		explicit MemoryArenaGrowing(uintPtr addressSize, uintPtr initialSize = 1024 * 1024) : origin(nullptr), pageSize(detail::memoryPageSize()), addressSize(detail::roundUpTo(addressSize, pageSize)), currentSize(0)
		{
			memory = newVirtualMemory();
			origin = memory->reserve(this->addressSize / pageSize);
			currentSize = detail::roundUpTo(initialSize <= addressSize ? initialSize : addressSize, pageSize);
			if (currentSize)
				memory->increase(currentSize / pageSize);
			allocator.setOrigin(origin);
			allocator.setSize(currentSize);
		}

		~MemoryArenaGrowing()
		{
			ScopeLock<ConcurrentPolicy> g(&concurrent);
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			ScopeLock<ConcurrentPolicy> g(&concurrent);
			try
			{
				return alloc(size, alignment);
			}
			catch (const OutOfMemory &e)
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
			catch (OutOfMemory &e)
			{
				e.severity = SeverityEnum::Error;
				e.makeLog();
				throw;
			}
		}

		void deallocate(void *ptr)
		{
			if (ptr == nullptr)
				return;
			ScopeLock<ConcurrentPolicy> g(&concurrent);
			allocator.deallocate(ptr);
		}

		void flush()
		{
			ScopeLock<ConcurrentPolicy> g(&concurrent);
			allocator.flush();
		}

		const void *getOrigin() const { return origin; }
		uintPtr getCurrentSize() const { return currentSize; }
		uintPtr getReservedSize() const { return addressSize; }

	private:
		Holder<VirtualMemory> memory;
		AllocatorPolicy allocator;
		ConcurrentPolicy concurrent;
		const uintPtr pageSize;
		void *origin;
		const uintPtr addressSize;
		uintPtr currentSize;

		void *alloc(uintPtr size, uintPtr alignment)
		{
			void *tmp = allocator.allocate(size, alignment);
			CAGE_ASSERT(tmp >= origin);
			CAGE_ASSERT((char*)tmp + size <= (char*)origin + currentSize);
			return tmp;
		}
	};

	template<class T>
	struct MemoryArenaStd
	{
		typedef T value_type;

		MemoryArenaStd(const MemoryArena &other) : a(other) {}
		template<class U>
		explicit MemoryArenaStd(const MemoryArenaStd<U> &other) : a(other.a) {}

		MemoryArenaStd(const MemoryArenaStd &) = default;
		MemoryArenaStd(MemoryArenaStd &&) = default;
		MemoryArenaStd &operator = (const MemoryArenaStd &) = default;
		MemoryArenaStd &operator = (MemoryArenaStd &&) = default;

		T *allocate(uintPtr cnt)
		{
			return (T*)a.allocate(cnt * sizeof(T), alignof(T));
		}

		void deallocate(T *ptr, uintPtr)
		{
			a.deallocate(ptr);
		}

		bool operator == (const MemoryArenaStd &other) const noexcept
		{
			return a == other.a;
		}

		bool operator != (const MemoryArenaStd &other) const noexcept
		{
			return a != other.a;
		}

	private:
		MemoryArena a;
		template<class U>
		friend struct MemoryArenaStd;
	};
}

#endif // guard_memoryAllocators_h_dsg4hsd564g6s5e1gse5a64fg
