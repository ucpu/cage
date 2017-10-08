namespace cage
{
	template<uintPtr Alignment = sizeof(uintPtr), class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
	struct memoryAllocatorPolicyLinear
	{
		memoryAllocatorPolicyLinear() : totalSize(0)
		{}

		void setOrigin(void *newOrigin)
		{
			CAGE_ASSERT_RUNTIME(!origin, "can only be set once", origin);
			current = origin = newOrigin;
		}

		void setSize(uintPtr size)
		{
			CAGE_ASSERT_RUNTIME(size >= current - origin, "size can not shrink", size, current, origin, totalSize);
			totalSize = size;
		}

		void *allocate(uintPtr size)
		{
			uintPtr alig = detail::addToAlign(current + BoundsPolicy::SizeFront, Alignment);
			uintPtr total = alig + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if (current + total > origin + totalSize)
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);

			pointer result = current + alig + BoundsPolicy::SizeFront;
			CAGE_ASSERT_RUNTIME((numeric_cast<uintPtr>(result.asVoid)) % Alignment == 0, "alignment failed", result, total, Alignment, current, alig, BoundsPolicy::SizeFront, size, BoundsPolicy::SizeBack);

			bound.setFront(result - BoundsPolicy::SizeFront);
			tag.set(result, size);
			bound.setBack(result + size);
			track.set(result, size);

			current += total;
			return result;
		}

		void deallocate(pointer ptr)
		{
			CAGE_THROW_CRITICAL(exception, "not allowed, must be flushed");
		}

		void flush()
		{
			track.flush();
			tag.check(origin, current - origin);
			current = origin;
		}

	private:
		BoundsPolicy bound;
		TaggingPolicy tag;
		TrackingPolicy track;
		pointer origin, current;
		uintPtr totalSize;
	};
}
