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
