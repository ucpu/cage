namespace cage
{
	template<class BoundsPolicy = memoryBoundsPolicyDefault, class TaggingPolicy = memoryTagPolicyDefault, class TrackingPolicy = memoryTrackPolicyDefault>
	struct memoryAllocatorPolicyLinear
	{
		memoryAllocatorPolicyLinear() : origin(nullptr), current(nullptr), totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "can only be set once", origin);
			current = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size >= numeric_cast<uintPtr>((char*)current - (char*)origin), "size can not shrink", size, current, origin, totalSize);
			totalSize = size;
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			uintPtr alig = detail::addToAlign((uintPtr)current + BoundsPolicy::SizeFront, alignment);
			uintPtr total = alig + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if ((char*)current + total > (char*)origin + totalSize)
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);

			void *result = (char*)current + alig + BoundsPolicy::SizeFront;
			CAGE_ASSERT_RUNTIME((uintPtr)result % alignment == 0, "alignment failed", result, total, alignment, current, alig, BoundsPolicy::SizeFront, size, BoundsPolicy::SizeBack);

			bound.setFront((char*)result - BoundsPolicy::SizeFront);
			tag.set(result, size);
			bound.setBack((char*)result + size);
			track.set(result, size);

			current = (char*)current + total;
			return result;
		}

		void deallocate(void *ptr)
		{
			CAGE_THROW_CRITICAL(exception, "not allowed, linear allocator must be flushed");
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

	template<uint8 N, class BoundsPolicy = memoryBoundsPolicyDefault, class TaggingPolicy = memoryTagPolicyDefault, class TrackingPolicy = memoryTrackPolicyDefault>
	struct memoryAllocatorPolicyNFrame
	{
		memoryAllocatorPolicyNFrame() : origin(nullptr), totalSize(0), current(0) {}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "nFrameAllocatorPolicy::setOrigin: can only be set once", origin);
			origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size > 0 && totalSize == 0, "nFrameAllocatorPolicy::setSize: can only be set once", origin, size, totalSize);
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
			CAGE_THROW_CRITICAL(exception, "not allowed, nFrame allocator must be flushed");
		}

		void flush()
		{
			current = (current + 1) % N;
			allocs[current].flush();
		}

	private:
		memoryAllocatorPolicyLinear<BoundsPolicy, TaggingPolicy, TrackingPolicy> allocs[N];
		void *origin;
		uintPtr totalSize;
		uint8 current;
	};

	template<uintPtr AtomSize, class BoundsPolicy = memoryBoundsPolicyDefault, class TaggingPolicy = memoryTagPolicyDefault, class TrackingPolicy = memoryTrackPolicyDefault>
	struct memoryAllocatorPolicyPool
	{
		CAGE_ASSERT_COMPILE(AtomSize > 0, objectsMustBeAtLeastOneByte);

		static constexpr uintPtr objectSizeInit()
		{
			uintPtr res = AtomSize + BoundsPolicy::SizeFront + BoundsPolicy::SizeBack;
			if (res < sizeof(uintPtr))
				res = sizeof(uintPtr);
			return res;
		}

		memoryAllocatorPolicyPool() : origin(nullptr), current(nullptr), totalSize(0), objectSize(objectSizeInit())
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "can only be set once", origin, newOrigin);
			CAGE_ASSERT_RUNTIME(newOrigin > (char*)objectSize, "origin too low", newOrigin, objectSize);
			uintPtr addition = detail::addToAlign((uintPtr)newOrigin, objectSize);
			if (addition < BoundsPolicy::SizeFront)
				addition += objectSize;
			current = origin = (char*)newOrigin + addition;
		}

		void setSize(uintPtr size)
		{
			if (size == 0 && totalSize == 0)
				return;
			CAGE_ASSERT_RUNTIME(size >= objectSize, "size must be at least the object size", size, objectSize);
			uintPtr s = size - objectSize - detail::subtractToAlign((uintPtr)origin + size, objectSize);
			if (s == totalSize)
				return;
			CAGE_ASSERT_RUNTIME(s > totalSize, "size can not shrink", s, totalSize); // can only grow
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
			CAGE_ASSERT_RUNTIME(current >= origin && current <= (char*)origin + totalSize, "current is corrupted", current, origin, totalSize);

			if (current >= (char*)origin + totalSize)
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", objectSize);

			void *next = *(void**)current;
			uintPtr alig = detail::addToAlign((uintPtr)current, alignment);
			CAGE_ASSERT_RUNTIME(size + alig <= AtomSize, "size with alignment too big", size, alignment, alig, AtomSize);
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
			CAGE_ASSERT_RUNTIME(ptr >= origin && ptr < (char*)origin + totalSize, "ptr must be element", ptr, origin, totalSize);
			ptr = (char*)ptr - detail::subtractToAlign((uintPtr)ptr, objectSize);
			CAGE_ASSERT_RUNTIME((uintPtr)ptr % objectSize == 0);

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

	template<class BoundsPolicy = memoryBoundsPolicyDefault, class TaggingPolicy = memoryTagPolicyDefault, class TrackingPolicy = memoryTrackPolicyDefault>
	struct memoryAllocatorPolicyQueue
	{
		memoryAllocatorPolicyQueue() : origin(nullptr), front(nullptr), back(nullptr), totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "can only be set once", origin);
			front = back = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size > 0 && totalSize == 0, "can only be set once", origin, size, totalSize);
			totalSize = size;
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			uintPtr alig = detail::addToAlign((uintPtr)back + sizeof(uintPtr) + BoundsPolicy::SizeFront, alignment);
			uintPtr total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if (back < front && (char*)back + total >(char*)front)
			{
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);
			}
			else if (back >= front)
			{
				if ((char*)back + total > (char*)origin + totalSize)
				{
					alig = detail::addToAlign((uintPtr)origin + sizeof(uintPtr) + BoundsPolicy::SizeFront, alignment);
					total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;
					if ((char*)origin + total > (char*)front)
						CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);
					back = origin;
				}
			}

			void *result = (char*)back + alig + sizeof(uintPtr) + BoundsPolicy::SizeFront;
			CAGE_ASSERT_RUNTIME((uintPtr)result % alignment == 0, "alignment failed", result, total, alignment, back, front, alig, BoundsPolicy::SizeFront, size, BoundsPolicy::SizeBack);

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
			CAGE_ASSERT_RUNTIME(ptr >= origin && ptr < (char*)origin + totalSize, "ptr must be element", ptr, origin, totalSize);

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

	template<class BoundsPolicy = memoryBoundsPolicyDefault, class TaggingPolicy = memoryTagPolicyDefault, class TrackingPolicy = memoryTrackPolicyDefault>
	struct memoryAllocatorPolicyStack
	{
		memoryAllocatorPolicyStack() : origin(nullptr), current(nullptr), totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "can only be set once", origin);
			current = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size >= numeric_cast<uintPtr>((char*)current - (char*)origin), "size can not shrink", size, current, origin, totalSize);
			totalSize = size;
		}

		void *allocate(uintPtr size, uintPtr alignment)
		{
			uintPtr alig = detail::addToAlign((uintPtr)current + sizeof(uintPtr) + BoundsPolicy::SizeFront, alignment);
			uintPtr total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if ((char*)current + total > (char*)origin + totalSize)
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);

			void *result = (char*)current + alig + sizeof(uintPtr) + BoundsPolicy::SizeFront;
			CAGE_ASSERT_RUNTIME((uintPtr)result % alignment == 0, "alignment failed", result, total, alignment, current, alig, BoundsPolicy::SizeFront, size, BoundsPolicy::SizeBack);

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
			CAGE_ASSERT_RUNTIME(ptr >= origin && ptr < (char*)origin + totalSize, "ptr must be element", ptr, origin, totalSize);

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
}
