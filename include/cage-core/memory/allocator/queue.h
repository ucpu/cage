namespace cage
{
	template<uintPtr Alignment = sizeof(uintPtr), class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
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

		void *allocate(uintPtr size)
		{
			uintPtr alig = detail::addToAlign((uintPtr)back + sizeof(uintPtr) + BoundsPolicy::SizeFront, Alignment);
			uintPtr total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;

			if (back < front && (char*)back + total > (char*)front)
			{
				CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);
			}
			else if (back >= front)
			{
				if ((char*)back + total > (char*)origin + totalSize)
				{
					alig = detail::addToAlign((uintPtr)origin + sizeof(uintPtr) + BoundsPolicy::SizeFront, Alignment);
					total = alig + sizeof(uintPtr) + BoundsPolicy::SizeFront + size + BoundsPolicy::SizeBack;
					if ((char*)origin + total > (char*)front)
						CAGE_THROW_SILENT(outOfMemoryException, "out of memory", total);
					back = origin;
				}
			}

			void *result = (char*)back + alig + sizeof(uintPtr) + BoundsPolicy::SizeFront;
			CAGE_ASSERT_RUNTIME((uintPtr)result % Alignment == 0, "alignment failed", result, total, Alignment, back, front, alig, BoundsPolicy::SizeFront, size, BoundsPolicy::SizeBack);

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
}
