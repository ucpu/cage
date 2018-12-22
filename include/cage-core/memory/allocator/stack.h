namespace cage
{
	template<class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
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
