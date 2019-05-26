namespace cage
{
#if defined (CAGE_DEBUG)
#define GCHL_DEFAULT_MEMORY_BOUNDS_POLICY memoryBoundsPolicySimple
#define GCHL_DEFAULT_MEMORY_TAG_POLICY memoryTagPolicySimple
#define GCHL_DEFAULT_MEMORY_TRACK_POLICY memoryTrackPolicySimple
#else
#define GCHL_DEFAULT_MEMORY_BOUNDS_POLICY memoryBoundsPolicyNone
#define GCHL_DEFAULT_MEMORY_TAG_POLICY memoryTagPolicyNone
#define GCHL_DEFAULT_MEMORY_TRACK_POLICY memoryTrackPolicyNone
#endif
}
namespace cage
{
	struct CAGE_API memoryBoundsPolicyNone
	{
		static const uintPtr SizeFront = 0;
		static const uintPtr SizeBack = 0;

		void setFront(void *ptr)
		{}

		void setBack(void *ptr)
		{}

		void checkFront(void *ptr)
		{}

		void checkBack(void *ptr)
		{}
	};

	struct CAGE_API memoryBoundsPolicySimple
	{
		static const uintPtr SizeFront = 4;
		static const uintPtr SizeBack = 4;

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
				CAGE_THROW_CRITICAL(exception, "memory corruption detected");
		}

		void checkBack(void *ptr)
		{
			if (*(uint32*)ptr != PatternBack)
				CAGE_THROW_CRITICAL(exception, "memory corruption detected");
		}

	private:
		static const uint32 PatternFront = 0xCDCDCDCD;
		static const uint32 PatternBack = 0xDCDCDCDC;
	};
}
namespace cage
{
	struct CAGE_API memoryConcurrentPolicyNone
	{
		void lock() {}
		void unlock() {}
	};

	struct CAGE_API memoryConcurrentPolicyMutex
	{
		memoryConcurrentPolicyMutex();
		void lock();
		void unlock();

	private:
		holder<void> mutex;
	};
}
namespace cage
{
	struct CAGE_API memoryTagPolicyNone
	{
		void set(void *ptr, uintPtr size)
		{}

		void check(void *ptr, uintPtr size)
		{}
	};

	struct CAGE_API memoryTagPolicySimple
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
}
namespace cage
{
	struct CAGE_API memoryTrackPolicyNone
	{
		void set(void *ptr, uintPtr size)
		{}

		void check(void *ptr)
		{}

		void flush()
		{}
	};

	struct CAGE_API memoryTrackPolicySimple
	{
		memoryTrackPolicySimple() : count(0)
		{}

		~memoryTrackPolicySimple();

		void set(void *ptr, const uintPtr size)
		{
			count++;
		}

		void check(void *ptr)
		{
			if (count == 0)
				CAGE_THROW_CRITICAL(exception, "double deallocation detected");
			count--;
		}

		void flush()
		{
			count = 0;
		}

	private:
		uint32 count;
	};

	struct CAGE_API memoryTrackPolicyAdvanced
	{
	public:
		memoryTrackPolicyAdvanced();
		~memoryTrackPolicyAdvanced();
		void set(void *ptr, uintPtr size);
		void check(void *ptr);
		void flush();
	private:
		void *data;
	};
}
