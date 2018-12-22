namespace cage
{
	template<class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
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
			CAGE_THROW_CRITICAL(exception, "not allowed, must be flushed");
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
