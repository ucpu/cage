namespace cage
{
	template<uintPtr AtomSize, bool StoreSize = GCHL_DEFAULT_MEMORY_STORE_SIZE, uintPtr Alignment = sizeof(uintPtr), class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
	struct memoryAllocatorPolicyPool
	{
		CAGE_ASSERT_COMPILE(AtomSize > 0, objectsMustBeAtLeastOneByte);
		CAGE_ASSERT_COMPILE(Alignment == 1 || (Alignment != 0 && Alignment % sizeof(uintPtr) == 0), alignmentMustBeDivisibleBySizeOfPointer);

		static uintPtr objectSizeInit()
		{
			uintPtr res = AtomSize + BoundsPolicy::SizeFront + BoundsPolicy::SizeBack;
			if (StoreSize || res < sizeof(uintPtr))
				res += sizeof(uintPtr);
			res += detail::addToAlign(pointer(res), Alignment);
			return res;
		}

		memoryAllocatorPolicyPool() : totalSize(0), objectSize(objectSizeInit())
		{
			CAGE_ASSERT_RUNTIME(objectSize < detail::memoryPageSize(), "objects too big", objectSize, detail::memoryPageSize());
		}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "can only be set once", origin, newOrigin);
			CAGE_ASSERT_RUNTIME(numeric_cast<uintPtr>(newOrigin) > objectSize, "origin too low", newOrigin, objectSize);
			uintPtr addition = detail::addToAlign(numeric_cast<uintPtr>(newOrigin), objectSize);
			if (addition < (StoreSize ? sizeof(uintPtr) : 0) + BoundsPolicy::SizeFront)
				addition += objectSize;
			current = origin = pointer(newOrigin) + addition;
		}

		void setSize(uintPtr size)
		{
			if (size == 0 && totalSize == 0)
				return;
			CAGE_ASSERT_RUNTIME(size >= objectSize, "size must be at least the object size", size, objectSize);
			uintPtr s = size - objectSize - detail::subtractToAlign(origin + size, objectSize);
			if (s == totalSize)
				return;
			CAGE_ASSERT_RUNTIME(s > totalSize, "size can not shrink", s, totalSize); // can only grow
			pointer tmp = origin + totalSize;
			totalSize = s;
			while (tmp < origin + totalSize)
			{
				*tmp.asPtr = tmp + objectSize;
				tmp += objectSize;
			}
		}

		void *allocate(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size <= AtomSize, "size can not exceed atom size", size, AtomSize);
			CAGE_ASSERT_RUNTIME(current >= origin && current <= origin + totalSize, "current is corrupted", current, origin, totalSize);

			if (current == origin + totalSize)
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", objectSize);

			pointer next = *current.asPtr;

			if (StoreSize)
				*(current - BoundsPolicy::SizeFront - sizeof(uintPtr)).asUintPtr = size;
			bound.setFront(current - BoundsPolicy::SizeFront);
			tag.set(current, StoreSize ? size : AtomSize);
			bound.setBack(current + (StoreSize ? size : AtomSize));
			track.set(current, size);

			void *result = current;
			current = next;
			return result;
		}

		void deallocate(pointer ptr)
		{
			CAGE_ASSERT_RUNTIME(ptr >= origin && ptr < origin + totalSize, "ptr must be element", ptr, origin, totalSize);
			CAGE_ASSERT_RUNTIME(ptr.decView % objectSize == 0, "ptr must be aligned", ptr, objectSize);

			track.check(ptr);
			bound.checkFront(ptr - BoundsPolicy::SizeFront);

			uintPtr size;
			if (StoreSize)
			{
				size = *(ptr - numeric_cast<uintPtr>(BoundsPolicy::SizeFront) - numeric_cast<uintPtr>(sizeof(uintPtr))).asUintPtr;
				CAGE_ASSERT_RUNTIME(size <= AtomSize, "restored size is not within range", size, AtomSize);

				tag.check(ptr, size);
				bound.checkBack(ptr + size);
			}
			else
			{
				tag.check(ptr, AtomSize);
				bound.checkBack(ptr + AtomSize);
			}

			*ptr.asPtr = current;
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
		pointer origin, current;
		uintPtr totalSize;
		const uintPtr objectSize;
	};
}
