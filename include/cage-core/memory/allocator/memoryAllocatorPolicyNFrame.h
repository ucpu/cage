namespace cage
{
	template<uint8 N, uintPtr Alignment = sizeof(uintPtr), class BoundsPolicy = GCHL_DEFAULT_MEMORY_BOUNDS_POLICY, class TaggingPolicy = GCHL_DEFAULT_MEMORY_TAG_POLICY, class TrackingPolicy = GCHL_DEFAULT_MEMORY_TRACK_POLICY>
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

		void *allocate(uintPtr size)
		{
			return allocs[current].allocate(size);
		}

		void deallocate(void *ptr)
		{
			CAGE_THROW_CRITICAL(exception, "not allowed, must be flushed");
		}

		void flush()
		{
			current = (current + 1) % N;
			allocs[current].flush();
		}

	private:
		memoryAllocatorPolicyLinear<Alignment, BoundsPolicy, TaggingPolicy, TrackingPolicy> allocs[N];
		void *origin;
		uintPtr totalSize;
		uint8 current;
	};
}
