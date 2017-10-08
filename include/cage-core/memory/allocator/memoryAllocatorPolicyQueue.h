namespace cage
{
	template<uintPtr Alignment = sizeof(uintPtr), class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
	struct memoryAllocatorPolicyQueue
	{
		memoryAllocatorPolicyQueue() : totalSize(0)
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

		void *allocate(uintPtr size)
		{
			uintPtr alig = detail::addToAlign(back + sizeof(uintPtr) + BoundsPolicy::SizeFront, Alignment);
			uintPtr total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if (back < front && back + total > front)
			{
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);
			}
			else if (back >= front)
			{
				if (back + total > origin + totalSize)
				{
					alig = detail::addToAlign(origin + sizeof(uintPtr) + BoundsPolicy::SizeFront, Alignment);
					total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;
					if (origin + total > front)
						CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);
					back = origin;
				}
			}

			pointer result = back + alig + sizeof(uintPtr) + BoundsPolicy::SizeFront;
			CAGE_ASSERT_RUNTIME((numeric_cast<uintPtr>(result.asVoid) % Alignment) == 0, "alignment failed", result, total, Alignment, back, front, alig, BoundsPolicy::SizeFront, size, BoundsPolicy::SizeBack);

			*(result - BoundsPolicy::SizeFront - sizeof(uintPtr)).asUintPtr = size;
			bound.setFront(result - BoundsPolicy::SizeFront);
			tag.set(result, size);
			bound.setBack(result + size);
			track.set(result, size);

			back += total;
			return result;
		}

		void deallocate(pointer ptr)
		{
			CAGE_ASSERT_RUNTIME(ptr >= origin && ptr < origin + totalSize, "ptr must be element", ptr, origin, totalSize);

			track.check(ptr);

			uintPtr size = *(ptr - BoundsPolicy::SizeFront - sizeof(uintPtr)).asUintPtr;

			bound.checkFront(ptr - BoundsPolicy::SizeFront);
			tag.check(ptr, size);
			bound.checkBack(ptr + size);

			front = ptr + size + BoundsPolicy::SizeBack;

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
		pointer origin, front, back;
		uintPtr totalSize;
	};
}
