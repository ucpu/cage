namespace cage
{
	struct CAGE_API MemoryBoundsPolicyNone
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

	struct CAGE_API MemoryBoundsPolicySimple
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
				CAGE_THROW_CRITICAL(Exception, "memory corruption detected");
		}

		void checkBack(void *ptr)
		{
			if (*(uint32*)ptr != PatternBack)
				CAGE_THROW_CRITICAL(Exception, "memory corruption detected");
		}

	private:
		static const uint32 PatternFront = 0xCDCDCDCD;
		static const uint32 PatternBack = 0xDCDCDCDC;
	};

	struct CAGE_API MemoryTagPolicyNone
	{
		void set(void *ptr, uintPtr size)
		{}

		void check(void *ptr, uintPtr size)
		{}
	};

	struct CAGE_API MemoryTagPolicySimple
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

	struct CAGE_API MemoryTrackPolicyNone
	{
		void set(void *ptr, uintPtr size)
		{}

		void check(void *ptr)
		{}

		void flush()
		{}
	};

	struct CAGE_API MemoryTrackPolicySimple
	{
		MemoryTrackPolicySimple() : count(0)
		{}

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
		uint32 count;
	};

	struct CAGE_API MemoryTrackPolicyAdvanced
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

	struct CAGE_API MemoryConcurrentPolicyNone
	{
		void lock() {}
		void unlock() {}
	};

	struct CAGE_API MemoryConcurrentPolicyMutex
	{
		MemoryConcurrentPolicyMutex();
		void lock();
		void unlock();

	private:
		Holder<void> mutex;
	};

#ifdef CAGE_DEBUG
	typedef MemoryBoundsPolicySimple MemoryBoundsPolicyDefault;
	typedef MemoryTagPolicySimple MemoryTagPolicyDefault;
	typedef MemoryTrackPolicySimple MemoryTrackPolicyDefault;
#else
	typedef MemoryBoundsPolicyNone MemoryBoundsPolicyDefault;
	typedef MemoryTagPolicyNone MemoryTagPolicyDefault;
	typedef MemoryTrackPolicyNone MemoryTrackPolicyDefault;
#endif
}
