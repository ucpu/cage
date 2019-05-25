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
