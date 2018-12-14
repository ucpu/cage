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
			detail::memset(ptr, 0xEF, size);
		}

		void check(void *ptr, uintPtr size)
		{
			detail::memset(ptr, 0xFE, size);
		}
	};
}
